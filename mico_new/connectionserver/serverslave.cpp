#include "serverslave.h"
#include "config/configreader.h"
#include "protocoldef/protocol.h"
#include "Message/Message.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "serverinfo/serverid.h"
#include <string.h>
#include "utilsock.h"

ServerSlave::ServerSlave(ServerQueue *masterqueue)
    : m_masterqueue(masterqueue)
{
    m_eventbase = event_base_new();

    // timeout for queue poll every 10ms
    struct timeval tenms= {0,10};
    event *ev1 = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    ServerSlave::queuepoll,
                    this);
    event_add(ev1, &tenms);

    // check tcp connection to logical server every 20s
    // used by m_connectionTimeoutRollTimer->wheel
    struct timeval six = {20,0};
    event *ev2 = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    ServerSlave::checkConnTimeout,
                    this);

    event_add(ev2, &six);

    m_connectionTimeoutRollTimer = new WheelTimerPoller();
}

void ServerSlave::run()
{
    event_base_dispatch(m_eventbase);
}

ServerSlave::~ServerSlave()
{
    event_base_loopbreak(m_eventbase);
    event_base_free(m_eventbase);
    delete m_connectionTimeoutRollTimer;
}

void ServerSlave::queuepoll(int fd, short what, void *ctx)
{
    ServerSlave *s = (ServerSlave *)ctx;
    s->processMessage();
}

void ServerSlave::checkConnTimeout(int fd, short what, void *ctx)
{
    ServerSlave *s = (ServerSlave *)ctx;
    s->checkConnTimeoutHelp();
}

void ServerSlave::checkConnTimeoutHelp()
{
    m_connectionTimeoutRollTimer->wheel();
}

void ServerSlave::newConnection(NewConnectionMessage *ncm)
{
    ::writelog(InfoType, "Add connection: %d", ncm->connSessionId());
    int fd = ncm->fd();
    evutil_make_socket_nonblocking(ncm->fd()); // must noblocking
    struct bufferevent *be = bufferevent_socket_new(m_eventbase,
                                fd,
                                BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE);
    std::shared_ptr<SlaveContext> sc = std::make_shared<SlaveContext>();
    sc->server = this;
    sc->conn = std::make_shared<TcpConnection>();
    sc->conn->connid = ncm->connSessionId();
    sc->conn->logicalserverid = ncm->logicalserverid();
    sc->conn->state = TcpConnection::Conneccted;
    sc->conn->be = be;
    sc->conn->ct = TcpConnection::Client;
    sc->conn->setClientAddress(ncm->addr());
    std::weak_ptr<SlaveContext> weaksc = sc;
    sc->timeoutTimer = this->m_connectionTimeoutRollTimer->newTimer([this, weaksc](){
        this->connectionTimeout(weaksc);
    });

    bufferevent_setcb(be,
                      ServerSlave::readcb,
                      0,
                      ServerSlave::eventcb,
                      sc.get());

    bufferevent_enable(be, EV_READ | EV_WRITE);

    //sockaddr_in *si = (sockaddr_in *)ncm->addr();
    // push this connection to map so we can find it
    //char strip[1024];
    //evutil_inet_ntop(AF_INET, &si->sin_addr, strip, sizeof(strip));
    //char ipportbuf[1024];
    //sprintf(ipportbuf, "%s:%d", strip, ntohs(si->sin_port));
    std::string ipport = ncm->addr().toString();
    ::writelog(InfoType, "new connection: %s", ipport.c_str());

    this->m_connections.insert(std::make_pair(ncm->connSessionId(), sc));

    //// create ctx
    //// std::string ipport(ipportbuf);
    //std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>();
    //conn->be = be;
    //conn->clientaddrlen = len;
    //conn->clientaddr = (sockaddr *)new char[len];
    //memcpy(conn->clientaddr, addr, len);
    //conn->logicalserverid = s->getLogicalServerId();
    //conn->ct = TcpConnection::Client;
    //conn->connid = genconnid();
    //s->insertConnection(conn->connid, conn);
    //newctx->conn = conn; // save conn to ctx
    //s->m_beip[be] = ipport;
    //bufferevent_get
}

void ServerSlave::sendToClient(SendToClientMessage *scm)
{
    std::shared_ptr<SlaveContext> sc = m_connections[scm->connSessionId()];
    if (sc)
    {
        sc->conn->send(scm->data().data(), scm->data().size());
#if 1
        ::writelog(InfoType, "send to size: %d, connid: %d",
                   scm->data().size(),
                   sc->conn->connid);
#endif
    }
    else
    {
        printf("connection %d not exist.\n", scm->connSessionId());
    }
}

void ServerSlave::setConnectionId(uint64_t connid, const TcpClientMessage &tcm)
{

    std::shared_ptr<SlaveContext> connctx = m_connections[connid];
    if (!connctx)
    {
        assert(false);
        return;
    }
    uint64_t cid = 0;
    uint32_t logicalserverid = 0;
    uint8_t answercode = ANSC::SUCCESS;
    // decrypt
    if (tcm.datalen() == 1)
    {
        //
        logicalserverid = connctx->conn->logicalserverid;
        cid = connid;
        connctx->conn->state = TcpConnection::Valid;
    }
    // update
    else if (tcm.datalen() == 1 + 4 + 8)
    {
        connctx->conn->state = TcpConnection::Valid;
        logicalserverid = ::ReadUint32(tcm.data() + 1);
        cid = ::ReadUint64(tcm.data() + 1 + 4);
        connctx->conn->connid = cid;
        connctx->conn->logicalserverid = logicalserverid;
        if (cid != connid)
        {
            // insert new
            m_connections[cid] = connctx;
            // remove old
            m_connections.erase(connid);
        }
        else
        {
            ::writelog(InfoType, "same connid: %lu", connid);
        }

        // send a message to master
        ChangeConnectionIdMessage *c = new ChangeConnectionIdMessage;
        c->oldconnid = connid;
        c->newconnid = cid;
        c->slaveserver = this;
        this->m_masterqueue->addMessage(c);
    }
    else
    {
        // ???
        answercode = ANSC::MESSAGE_ERROR;
        ::writelog(InfoType, "set connid error.%d", connid);
    }
    // send to client
    Message reply;
    reply.setPrefix(CLIENT_MSG_PREFIX);
    reply.setSuffix(CLIENT_MSG_SUFFIX);
    reply.setSerialNumber(tcm.ser());
    reply.setCommandID(CMD::SET_TCP_SESSION_ID);
    reply.appendContent(answercode);
    if (answercode == ANSC::SUCCESS)
    {
        reply.appendContent(uint8_t(4 + 8)); // length of sid
        reply.appendContent(uint32_t(logicalserverid));
        reply.appendContent(uint64_t(cid));
    }
    else
    {
        reply.appendContent(uint8_t(0));
    }
    std::vector<char> out;
    reply.toByteArray(&out);
    connctx->conn->send(out.data(), out.size());
}

// a connection timeout so close it
void ServerSlave::connectionTimeout(std::weak_ptr<SlaveContext> weaksc)
{
    if (weaksc.expired())
        return;

    uint64_t connid = 0;
    {
        connid = weaksc.lock()->conn->connid;
    }

    ::writelog(InfoType, "connection timeout: %lu", connid);
    // send a message to master
    ClientConnectionClosedMessage *clientclose = new ClientConnectionClosedMessage;
    clientclose->connid = connid;
    clientclose->slaveserver = this;
    this->m_masterqueue->addMessage(clientclose);
    this->m_connections.erase(connid);
}

void ServerSlave::readcb(bufferevent *bev, void *ctx)
{
    SlaveContext *sc = (SlaveContext *)ctx;
    Configure *config = Configure::getConfig();

    evbuffer *buf = bufferevent_get_input(bev);
    int len = evbuffer_get_length(buf);
    std::vector<char> vbuf(len);
    char *p = &vbuf[0];
    int plen = vbuf.size();
    int readlen = evbuffer_copyout(buf, p, plen);
    assert(readlen == plen);(void)readlen;
    std::vector<char> out;
    int conntype = sc->conn->ct;
    int lenparsed = 0;
    //::writelog(InfoType, "Read data connid: %d, size:%d", sc->conn->connid, len);
    if (conntype == TcpConnection::Client)
    {
        TcpClientMessage tcm;
        int onepacklen = 0;
        while (tcm.fromBtyeArray(p + lenparsed, len - lenparsed, &onepacklen))
        {
            if (tcm.cmd() == CMD::SET_TCP_SESSION_ID)
            {
                ::writelog(InfoType, "SET tco session id %lu, %u", sc->conn->connid, tcm.ser());
                // process set session id
                sc->server->setConnectionId(sc->conn->connid, tcm);
            }
            else if (sc->conn->state == TcpConnection::Valid)
            {
                // process tcm
                TcpServerMessage tologicalserver;
                tologicalserver.cmd = TcpServerMessage::NewMessage;
                tologicalserver.connServerId = config->serverid;
                tologicalserver.connid = sc->conn->connid;
                tologicalserver.addrlen = sc->conn->getClientaddr().getAddressLen();
                tologicalserver.clientaddr = sc->conn->getClientaddr().getAddress();
                tologicalserver.clientmessage = p + lenparsed;
                tologicalserver.clientmessageLength = onepacklen;

                // send message to master, master will send the client msg to logicalserver
                std::vector<char> d = tologicalserver.toByteArray();
                SendToServerMessage *toservermsg = new SendToServerMessage;
                toservermsg->connid = sc->conn->connid;
                toservermsg->logicalServerid = sc->conn->logicalserverid;
                toservermsg->data = d;
                //
#if 1
                ::writelog(InfoType, "tcp recv msg, logicalid: %d, connid: %lu, cmd: %u",
                     sc->conn->logicalserverid,
                     sc->conn->connid, tcm.cmd());
#endif

                if (isServerID(tcm.destId())
                        && toservermsg->logicalServerid != tcm.destId())
                {
                    //
                    toservermsg->logicalServerid = tcm.destId();
                    ::writelog(InfoType, "change logical server id to %d",
                               toservermsg->logicalServerid);
                }
                // update timer
                sc->server->m_connectionTimeoutRollTimer->update(
                            sc->timeoutTimer);

                sc->server->m_masterqueue->addMessage(toservermsg);
            }
            else
            {
                // drop msg
                ::writelog(InfoType, "conn no valid.");
            }
            lenparsed += onepacklen;

            onepacklen = 0;
        }
        lenparsed += onepacklen;
    }
    else if (conntype == TcpConnection::Server)
    {
        assert(false);
    }
    else
    {
        assert(false);
    }
    evbuffer_drain(buf, lenparsed);
}

void ServerSlave::eventcb(bufferevent *bev, short events, void *ctx)
{
    SlaveContext *sc = (SlaveContext *)ctx;
    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        ::writelog(InfoType, "Got an error %d (%s) on the slave. "
                "event: %d, connid: %lu.",
                   err,
                   evutil_socket_error_to_string(err),
                   events,
                   sc->conn->connid);
        // should close connection ????
        uint64_t connid = sc->conn->connid;
        ClientConnectionClosedMessage *clientclose = new ClientConnectionClosedMessage;
        clientclose->connid = connid;
        clientclose->slaveserver = sc->server;
        sc->server->m_masterqueue->addMessage(clientclose);
        sc->server->m_connections.erase(connid);
    }
    else if (events & BEV_EVENT_EOF)
    {
        // 这里是多线程不安全的, 但是因为只有一个线程, 所以不会有问题
        // connectin closed
        ::writelog(InfoType, "connection closed, serverid :%d, conn: %d",
               sc->conn->logicalserverid,
               sc->conn->connid);
        //cc->conn->state = TcpConnection::Invalid;
        //bufferevent_free(cc->conn->be);
        //cc->conn->be = nullptr;
        uint64_t connid = sc->conn->connid;
        // send a message to master
        ClientConnectionClosedMessage *clientclose = new ClientConnectionClosedMessage;
        clientclose->connid = connid;
        clientclose->slaveserver = sc->server;
        sc->server->m_masterqueue->addMessage(clientclose);

        sc->server->m_connections.erase(connid);
    }
    else
    {
        //
    }
}

void ServerSlave::addMessage(IConnectionServerMessage *msg)
{
    m_queue.addMessage(msg);
}

void ServerSlave::processMessage()
{
    std::list<IConnectionServerMessage*> msgs;
    m_queue.getMessage(&msgs);
    for (IConnectionServerMessage *msg : msgs)
    {
        switch (msg->type())
        {
        case NewConnectionMessageType:
        {
            NewConnectionMessage *ncm = (NewConnectionMessage *)msg;
            this->newConnection(ncm);
        }
        break;

        case SendToClientMessageType:
        {
            this->sendToClient((SendToClientMessage *)msg);
        }
        break;

        default:
            assert(false);
            break;
        }

        delete msg;
    }
}
