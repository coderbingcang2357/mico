#include <stdio.h>
#include <string.h>
#include "connectionserver.h"
#include "connidgen.h"
#include "tcpclientmessage.h"
#include "Message/tcpservermessage.h"
#include "config/configreader.h"
#include "util/logwriter.h"

static void write_to_file_cb(int severity, const char *msg)
{
    const char *s;
    switch (severity) {
        case _EVENT_LOG_DEBUG: s = "debug"; break;
        case _EVENT_LOG_MSG:   s = "msg";   break;
        case _EVENT_LOG_WARN:  s = "warn";  break;
        case _EVENT_LOG_ERR:   s = "error"; break;
        default:               s = "?";     break; /* never reached */
    }
    printf("[%s] %s\n", s, msg);
}

/* Redirect all Libevent log messages to the C stdio file 'f'. */
void set_logfile(FILE *f)
{
    (void)f;
    //logfile = f;
    event_set_log_callback(write_to_file_cb);
}


ConnectionServer::ConnectionServer(const std::string &bindaddr, uint16_t port, int threadcnt)
    : m_bindaddr(bindaddr), m_port(port), m_udpserver(&m_queue)
{
    int res = evthread_use_pthreads();
    assert(res == 0);

    set_logfile(0);
    m_eventbase = event_base_new();

    // timeout for queue poll every 10ms
    struct timeval tenms= {0,10 * 1000};
    event *ev1 = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    ConnectionServer::queuepoll,
                    this);
    event_add(ev1, &tenms);
    // check tcp connection to logical server every 60s
    struct timeval six = {15,0};
    event *ev2 = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    ConnectionServer::connectToLogicalServerPoll,
                    this);

    event_add(ev2, &six);
    // heart beat timer
    struct timeval hbtimer = {60 * 5, 0};
    //struct timeval hbtimer = {2, 0};
    event *hbev = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    ConnectionServer::heartbeatTimer,
                    this);

    event_add(hbev, &hbtimer);
}

ConnectionServer::~ConnectionServer()
{
    event_base_loopbreak(m_eventbase);
    event_base_free(m_eventbase);
    evconnlistener_free(m_listener);
    for (ServerSlave *ss : m_slaves)
    {
        delete ss;
    }
    for (Thread *th : m_slavethread)
    {
        delete th;
    }
    m_logicalServerConnections.clear();
    m_udpserver.quit();
    delete m_udpthread;
    delete m_listenctx;
}

void ConnectionServer::createListener()
{
    if (m_listenctx != nullptr)
        return;

    // create listen socket
    //sockaddr_in6 sock;
    //memset(&sock, 0, sizeof(sock));
    //sock.sin6_family = AF_INET6;
    //memset(sock.sin6_addr.s6_addr, 0, sizeof(sock));
    //sock.sin6_port = htons(m_port);

    sockaddr_in sock;
    memset(&sock, 0, sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = htonl(0);
    sock.sin_port = htons(m_port);
    //sock.sin_port = htons(m_port);
    // creat evlistener
    m_listenctx = new ConnCtx;
    m_listenctx->server = this;

    //
    m_listener = evconnlistener_new_bind(m_eventbase,
        ConnectionServer::acceptcb,
        m_listenctx,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1,
        (sockaddr *)&sock,
        sizeof(sock));
    evconnlistener_set_error_cb(m_listener, listererErrorcb);
}

int ConnectionServer::run()
{
    // create slave
    for (int i = 0; i < m_threadcnt; ++i)
    {
        ServerSlave *slave = new ServerSlave(&this->m_queue);
        m_slaves.push_back(slave);
        Thread *slavethread = new Thread([slave](){
            slave->run();
        });
        m_slavethread.push_back(slavethread);
    }
    // udp
    m_udpthread  = new Thread([this](){
        m_udpserver.run();
    });

    printf("Running...");
    struct timeval t = {0, 10};
    event *ev2 = event_new(m_eventbase,
                    -1,
                    EV_TIMEOUT,
                    ConnectionServer::connectToLogicalServerAtStart,
                    this);

    event_add(ev2, &t);
    event_base_dispatch(m_eventbase);
    return 0;
}

void ConnectionServer::writelogcb(int severity, const char *msg)
{

}

void ConnectionServer::acceptcb(evconnlistener *listener, int fd, sockaddr *addr, int len, void *ctx)
{
    ConnCtx *connctx = (ConnCtx *)ctx;
    ConnectionServer *s = connctx->server;
    assert(ctx == s->m_listenctx);
    NewConnectionMessage *ncm = new NewConnectionMessage;
    ncm->setFd(fd);
    ncm->setAddress(addr, len);
    ncm->setConnSessionId(genconnid());
    ncm->setLogicalserverid(s->getLogicalServerId());
    ServerSlave *slave = s->allocSlave();
    slave->addMessage(ncm);
    s->m_connidMapSlave.insert(std::make_pair(ncm->connSessionId(), slave));

}

void ConnectionServer::listererErrorcb(evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    printf("Got an error %d (%s) on the listener.Shutting down.\n",
            err,
            evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

void ConnectionServer::readcb(bufferevent *bev, void *ctx)
{
    ConnCtx *cc = (ConnCtx *)ctx;
    assert(cc->conn->be == bev);
    ConnectionServer *tcpserver = cc->server;

    evbuffer *buf = bufferevent_get_input(bev);
    int len = evbuffer_get_length(buf);
    std::vector<char> vbuf(len);
    char *p = &vbuf[0];
    int plen = vbuf.size();
    int readlen = evbuffer_copyout(buf, p, plen);
    assert(readlen == plen);(void)readlen;
    std::vector<char> out;
    int conntype = cc->conn->ct;
    int lenparsed = 0;
    if (conntype == TcpConnection::Client)
    {
        assert(false);
    }
    else if (conntype == TcpConnection::Server)
    {
        TcpServerMessage tsm;
        int onepacklen = 0;
        while (tsm.fromByteArray(p + lenparsed, len - lenparsed, &onepacklen))
        {
            lenparsed += onepacklen;

            if (tsm.cmd == TcpServerMessage::NewMessage)
            {
                // Tcp client
                if (tsm.connid != 0)
                {
                    ServerSlave *slave = tcpserver->findConnectionSlave(tsm.connid);
                    if (slave != nullptr)
                    {
                        SendToClientMessage *scm = new SendToClientMessage;
                        scm->setConnSessionId(tsm.connid);
                        scm->setData(std::vector<char>(tsm.clientmessage, tsm.clientmessage + tsm.clientmessageLength));

                        slave->addMessage(scm);
                    }
                    else
                    {
                        ::writelog(InfoType, "connection %ld not exist.", tsm.connid);
                    }
                }
                // udp client
                else
                {
                    UdpSendToClientMessage *udpmsg = new UdpSendToClientMessage;
                    udpmsg->setAddr(tsm.clientaddr, tsm.addrlen);
                    udpmsg->setData(std::vector<char>(tsm.clientmessage, tsm.clientmessage + tsm.clientmessageLength));
                    cc->server->m_udpserver.addMessage(udpmsg);
                }
            }
            else if (tsm.cmd == TcpServerMessage::SetConnServerId)
            {
                cc->conn->state = TcpConnection::Valid;
                cc->server->m_logicalserid.push_back(cc->conn->logicalserverid);
                cc->server->createListener();
                ::writelog(InfoType, "conn to %d ok!!", cc->conn->logicalserverid);
            }
            else
            {
                assert(false);
            }

            onepacklen = 0;
        }
        lenparsed += onepacklen;
    }
    else
    {
        assert(false);
    }
    evbuffer_drain(buf, lenparsed);
}

//void ConnectionServer::writecb(bufferevent *bev, void *ctx)
//{
//
//}

void ConnectionServer::eventcb(bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR)
    {
        ConnCtx *connctx = (ConnCtx *)ctx;
        int err = EVUTIL_SOCKET_ERROR();
        ::writelog(InfoType,
                   "Got an error %d (%s) on the master. "
                    "events: %d, connid: %lu, logicalserverid: %d",
                    err,
                    evutil_socket_error_to_string(err),
                    events,
                    connctx->conn->connid,
                    connctx->conn->logicalserverid);
    }
    else if (events & BEV_EVENT_EOF)
    {
        // 这里是多线程不安全的, 但是因为只有一个线程, 所以不会有问题
        // connectin closed
        ConnCtx *cc = (ConnCtx *)ctx;
        assert(cc->conn->ct == TcpConnection::Server);
        ::writelog(InfoType, "connection closed, serverid :%d", cc->conn->logicalserverid);
        cc->conn->state = TcpConnection::Invalid;
        bufferevent_free(cc->conn->be);
        cc->conn->be = nullptr;
    }
    else
    {
        //
    }
}

void ConnectionServer::connectcb(bufferevent *bev, short events, void *ctx)
{
    ConnCtx *connctx = (ConnCtx*)ctx;
    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        ::writelog(InfoType,
                   "master Got an error %d (%s) on the connect to server id %d."
                   "connid: %lu, events: %d",
                    err,
                    evutil_socket_error_to_string(err),
                    connctx->conn->logicalserverid,
                    connctx->conn->connid,
                    events);

        connctx->conn->state = TcpConnection::Invalid;
    }
    else if (events & (BEV_EVENT_CONNECTED))
    {
        connctx->conn->state = TcpConnection::Conneccted;
        ::writelog(InfoType,
                   "connect ok.to %ld, %d, %x",
                   connctx->conn->connid,
               connctx->conn->logicalserverid, events);
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        bufferevent_setcb(bev,
                          ConnectionServer::readcb,
                          0,
                          ConnectionServer::eventcb,
                          ctx);
        connctx->server->setConnectionServerId(connctx);
    }
    else
    {
        assert(false);
    }
}

void ConnectionServer::queuepoll(int fd, short what, void *ctx)
{
    assert(what | EV_TIMEOUT);
    // printf("queuepoll timeout.\n");
    ConnectionServer *s = (ConnectionServer *)ctx;
    s->processQueueMessage();
}

void ConnectionServer::connectToLogicalServerPoll(int fd, short what, void *ctx)
{
    ConnectionServer *s = (ConnectionServer *)ctx;
    s->connectToLogicalServerPollHelp();
}

void ConnectionServer::connectToLogicalServerAtStart(int fd, short what, void *ctx)
{
    ::writelog(InfoType, "connect to logical server.");
    ConnectionServer *s = (ConnectionServer *)ctx;
    s->connectToLogicalServer();
}

void ConnectionServer::heartbeatTimer(int fd, short what, void *ctx)
{
    ConnectionServer *s = (ConnectionServer *)ctx;
    s->sendHeartBeat();
}

int ConnectionServer::getLogicalServerId()
{
    if (m_logicalserid.size() == 0)
        return -1;

    int32_t id = m_logicalserid.back();
    m_logicalserid.pop_back();
    m_logicalserid.push_front(id);
    ::writelog(InfoType, "alloc logical server:%d", id);
    return id;
}

ServerSlave *ConnectionServer::allocSlave()
{
    ServerSlave *ss;
    if (m_slaves.size() > 1)
    {
        ss = m_slaves.back();
        m_slaves.pop_back();
        m_slaves.push_front(ss);
    }
    else
    {
        ss = m_slaves.back();
    }
    return ss;
}

ServerSlave *ConnectionServer::findConnectionSlave(uint64_t connid)
{
    auto it = m_connidMapSlave.find(connid);
    if (it != m_connidMapSlave.end())
    {
        return it->second;
    }
    else
    {
        return nullptr;
    }
}

void ConnectionServer::processQueueMessage()
{
    //Configure *c = Configure::getConfig();
    std::list<IConnectionServerMessage*> msgs;
    this->m_queue.getMessage(&msgs);
    for (IConnectionServerMessage *msg : msgs)
    {
        switch (msg->type())
        {
        case SendToServerMessageType:
        {
            SendToServerMessage *ssm = (SendToServerMessage *)msg;

            //TcpServerMessage tsm;
            //tsm.connid = ssm->connid;
            //tsm.cmd = TcpServerMessage::NewMessage;
            //tsm.connServerId = c->serverid;
            //tsm.clientmessageLength = ssm->data.size();
            //tsm.clientmessage = ssm->data.data();
            //std::vector<char> d = tsm.toByteArray();
            auto it = m_logicalServerConnections.find(ssm->logicalServerid);
            std::shared_ptr<ConnCtx> connctx;
            if (it != m_logicalServerConnections.end())
            {
                connctx = it->second;
            }
            if (connctx && connctx->conn->state == TcpConnection::Valid)
            {
                connctx->conn->send(ssm->data.data(), ssm->data.size());
            }
            else
            {
                //
                ::writelog(InfoType,
                           "error send to server %d, connection not valid\n",
                           ssm->logicalServerid);
            }
        }
        break;

        case UdpSendToServerMessageType:
        {
            UdpSendToServerMessage *udpmsg = (UdpSendToServerMessage *)msg;
            int logicalid = 0;
            if (udpmsg->logicalserverid == 0)
            {
                auto it = m_udpconnection.find(udpmsg->ipport);
                if (it == m_udpconnection.end())
                {
                    logicalid = this->getLogicalServerId();
                    if (logicalid != -1)
                    {
                        m_udpconnection.insert(std::make_pair(udpmsg->ipport,
                                                              logicalid));
                    }
                    else
                    {
                        //assert(false);
                        ::writelog(ErrorType, "No logical server avalable");
                        goto breakend;
                    }
                }
                else
                {
                    logicalid = it->second;
                }
            }
            else
            {
                logicalid = udpmsg->logicalserverid;
            }

            std::shared_ptr<ConnCtx> connctx = m_logicalServerConnections[logicalid];
            if (connctx && connctx->conn->state == TcpConnection::Valid)
            {
                connctx->conn->send(udpmsg->data.data(), udpmsg->data.size());
            }
            else
            {
                //
                ::writelog(InfoType, "udp error send to server %d, connection not valid",
                       logicalid);
            }
        }
        breakend:
        break;

        case ClientConnectionClosedMessageType:
        {
            ClientConnectionClosedMessage *m = (ClientConnectionClosedMessage *)msg;
            auto it = m_connidMapSlave.find(m->connid);
            if (it != m_connidMapSlave.end() && it->second == m->slaveserver)
            {
                m_connidMapSlave.erase(m->connid);
                ::writelog(InfoType, "master remove conn: %lu", m->connid);
            }
        }
        break;

        case ChangeConnectionIdMessageType:
        {
            ChangeConnectionIdMessage *m = (ChangeConnectionIdMessage *)msg;
            m_connidMapSlave.erase(m->oldconnid);
            //m_connidMapSlave.insert(std::make_pair(m->newconnid,
            //                               (ServerSlave *)(m->slaveserver)));
            m_connidMapSlave[m->newconnid] =  (ServerSlave *)(m->slaveserver);
        }
        break;

        default:
            assert(false);
            break;
        }

        delete msg;
    }
}

void ConnectionServer::connectToLogicalServer()
{
    auto servers = m_logicalservers.getServers();
    for (auto &p : servers)
    {
        LogicalServer &ls = p.second;
        // connect to server.
        sockaddr_in sock;
        int len = sizeof(sock);
        memset(&sock, 0, sizeof(sock));
        std::string ipport = ls.ip + ":" + std::to_string(ls.port);
        if (evutil_parse_sockaddr_port(ipport.c_str(),
                (struct sockaddr *)&sock,
                &len) == -1)
        {
            ::writelog(InfoType, "adresss error.\n");
        }
        else
        {
            // connect to server and send data to .
            bufferevent *be = bufferevent_socket_new(m_eventbase,
                -1,
                BEV_OPT_CLOSE_ON_FREE
                | BEV_OPT_THREADSAFE);
            //
            std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>();
            conn->be = be;
            conn->logicalserverid = ls.serverid;
            conn->connid = genconnid();
            conn->ct = TcpConnection::Server;
            conn->state = TcpConnection::Connecting;

            std::shared_ptr<ConnCtx> ctx = std::make_shared<ConnCtx>();
            ctx->server = this;
            ctx->conn = conn;
            bufferevent_setcb(be,
                0,//Tcpserver::readcb,
                0, ConnectionServer::connectcb, ctx.get());
            //bufferevent_enable(be, EV_READ | EV_WRITE);
            bufferevent_socket_connect(be, (struct sockaddr *)&sock, len);
            //
            m_logicalServerConnections[conn->logicalserverid] = ctx;
            //bufferevent_write(be, p, len);
        }
    }
}

// send the serverid to Logical server
void ConnectionServer::setConnectionServerId(ConnCtx *connctx)
{
    Configure *c = Configure::getConfig();
    TcpServerMessage tsm;
    tsm.cmd = TcpServerMessage::SetConnServerId;
    tsm.connid = connctx->conn->connid;
    tsm.connServerId = c->serverid;
    std::vector<char> d = tsm.toByteArray();
    connctx->conn->send(d.data(), d.size());
}

void ConnectionServer::connectToLogicalServerPollHelp()
{
    for (auto &conn : m_logicalServerConnections)
    {
        std::shared_ptr<ConnCtx> connctx = conn.second;
        if (connctx->conn->state == TcpConnection::Invalid)
        {
            LogicalServer ls = m_logicalservers.get(connctx->conn->logicalserverid);
            sockaddr_in sock;
            int len = sizeof(sock);
            memset(&sock, 0, sizeof(sock));
            std::string ipport = ls.ip + ":" + std::to_string(ls.port);
            if (evutil_parse_sockaddr_port(ipport.c_str(),
                    (struct sockaddr *)&sock,
                    &len) == -1)
            {
                printf("adresss error 2.\n");
            }
            else
            {
                bufferevent *be = bufferevent_socket_new(m_eventbase,
                    -1,
                    BEV_OPT_CLOSE_ON_FREE
                    | BEV_OPT_THREADSAFE);
                //
                connctx->conn->be = be;
                // connctx->conn->logicalserverid = ls.serverid;
                // connctx->conn->connid = genconnid();
                connctx->conn->ct = TcpConnection::Server;
                connctx->conn->state = TcpConnection::Connecting;

                //std::shared_ptr<ConnCtx> ctx = std::make_shared<ConnCtx>();
                //ctx->server = this;
                //ctx->conn = conn;
                bufferevent_setcb(be,
                                  0,//Tcpserver::readcb,
                                  0,
                                  ConnectionServer::connectcb,
                                  connctx.get());
                //bufferevent_enable(be, EV_READ | EV_WRITE);
                bufferevent_socket_connect(be, (struct sockaddr *)&sock, len);
                //
                m_logicalServerConnections[connctx->conn->logicalserverid] =
                                                  connctx;
                //bufferevent_write(be, p, len);
            }

        }
    }
}

void ConnectionServer::sendHeartBeat()
{
    Configure *c = Configure::getConfig();
    TcpServerMessage tsm;
    tsm.cmd = TcpServerMessage::HeartBeat;
    tsm.connServerId = c->serverid;
    auto it =  m_logicalServerConnections.begin();
    for (; it != m_logicalServerConnections.end(); ++it)
    {
        if (it->second->conn->state == TcpConnection::ConnectState::Valid)
        {
            tsm.connid = it->second->conn->connid;
            std::vector<char> d = tsm.toByteArray();
            it->second->conn->send(d.data(), d.size());
            ::writelog(InfoType, "Send heart beat to server: %d", it->first);
        }
    }
}

void ConnectionServer::setLogicalservers(const LogicalServers &logicalservers)
{
    m_logicalservers = logicalservers;
}

