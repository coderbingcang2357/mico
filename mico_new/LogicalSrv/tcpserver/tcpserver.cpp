#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <event2/util.h>
#include "tcpserver.h"
#include "util/logwriter.h"
#include "util/sock2string.h"

class ServerThread : public Proc
{
public:
    ServerThread(Tcpserver *t) : m_tcpserver(t){}
    void run()
    {
        m_tcpserver->run();
    }
private:
    Tcpserver *m_tcpserver;
};

void Tcpserver::writelogcb(int severity, const char *msg)
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

Tcpserver::Tcpserver(const std::string &ip, uint16_t port, Tcpmessageprocessor *p)
    : m_ip(ip), m_port(port), processor(p)
{
}

Tcpserver::~Tcpserver()
{
    event_base_loopexit(m_evbase, NULL);
    event_base_free(m_evbase);
    delete processor;
}

void Tcpserver::send(const std::string &ipport, const std::vector<char> &data)
{
    WriteMessage *msg = new WriteMessage;
    msg->ipport = ipport;
    msg->data = data;
    m_queuelock.lock();
    m_writequeue.push_back(msg);
    m_queuelock.release();
    //std::shared_ptr<Tcpconnection> conn = m_manager.getConnection(ipport);
    //if (conn)
    //{
    //    conn->send(p, datalen);
    //}
    //else
    //{
    //    }
    //}
}

void Tcpserver::send(uint32_t connserverid,
                     uint64_t connid,
                     const sockaddrwrap &addr,
                     //int addrlen,
                     char *p,
                     int len)
{
    std::shared_ptr<ConnectionCtx> ctx = m_manager.getConnection(connserverid);
    if (ctx)
    {
        ctx->conn->connectionServerId();
        TcpServerMessage tsm;
        tsm.cmd = TcpServerMessage::NewMessage;
        tsm.connid = connid;
        tsm.connServerId = connserverid;//???;
        tsm.clientaddr = (sockaddr *)&addr;
        tsm.addrlen = addr.getAddressLen();
        tsm.clientmessage = p;
        tsm.clientmessageLength = len;
        std::vector<char> data = tsm.toByteArray();
        ctx->conn->send(data.data(), data.size());
    }
    else
    {
        // writelog
        ::writelog(InfoType, "connection server id %d not exist.", connserverid);
        //assert(false);
    }
}

void Tcpserver::create()
{
    ServerThread *s = new ServerThread(this);
    // thread will take ownership of s
    m_t = new Thread(s);
}

void Tcpserver::run()
{
    if (evthread_use_pthreads() == -1)
    {
        printf("use pthread error.\n");
        exit(0);
    }
    // init log
    event_set_log_callback(Tcpserver::writelogcb);

    // create event base
    m_evbase = event_base_new();
    printf("backend method: %s\n", event_base_get_method(m_evbase));
    // create listen sock
    sockaddr_in sock;
    memset(&sock, 0, sizeof(sock));
    sock.sin_family = AF_INET;
    sock.sin_addr.s_addr = htonl(0);
    sock.sin_port = htons(m_port);
    // creat evlistener
    m_listener = evconnlistener_new_bind(m_evbase,
        Tcpserver::acceptcb,
        this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1,
        (sockaddr *)&sock,
        sizeof(sock));
    evconnlistener_set_error_cb(m_listener, listererErrorcb);

    // queue poll
    // timeout for queue poll every 10ms
    struct timeval tenms= {0,10};
    event *ev1 = event_new(m_evbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    Tcpserver::queuepoll,
                    this);
    event_add(ev1, &tenms);

    // heartbeat
    struct timeval hbtimer = {60 * 5, 0};
    event *hbev = event_new(m_evbase,
                    -1,
                    EV_TIMEOUT | EV_PERSIST,
                    Tcpserver::heartBeatTimer,
                    this);
    event_add(hbev, &hbtimer);

    event_base_dispatch(m_evbase);
    printf("sever quit.\n");
}

void Tcpserver::acceptcb(evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *addr, int len, void *ctx)
{
    Tcpserver *s = (Tcpserver *)ctx;
    std::shared_ptr<ConnectionCtx> connctx =  std::make_shared<ConnectionCtx>();
    connctx->server = s;

    evutil_make_socket_nonblocking(fd);
    struct event_base *base = evconnlistener_get_base(listener);;
    struct bufferevent *be = bufferevent_socket_new(base,
        fd, BEV_OPT_THREADSAFE | BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(be, Tcpserver::readcb, 0, Tcpserver::eventcb, connctx.get());
    bufferevent_enable(be, EV_READ | EV_WRITE);
    connctx->conn = std::make_shared<Tcpconnection>(be);

    sockaddrwrap addrwrap;
    addrwrap.setAddress(addr, len);
    std::string ipport = addrwrap.toString();
    ::writelog(InfoType, "new connection: %s", ipport.c_str());

    s->m_manager.insertConnection(ipport, connctx);
    //s->m_beip[be] = ipport;
}

void Tcpserver::listererErrorcb(evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    printf("Got an error %d (%s) on the listener.Shutting down.\n",
            err,
            evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);
}

void Tcpserver::readcb(struct bufferevent *bev, void *ctx)
{
    //Tcpserver *tcpserver = (Tcpserver *)ctx;
    ConnectionCtx *connctx = (ConnectionCtx *)ctx;

    evbuffer *buf = bufferevent_get_input(bev);
    int len = evbuffer_get_length(buf);
    std::vector<char> vbuf(len);
    char *p = &vbuf[0];
    int plen = vbuf.size();
    int readlen = evbuffer_copyout(buf, p, plen);
    assert(readlen == plen);(void)readlen;
    int unpacklen = 0;
    //std::vector<char> out;
    int onepacklen = 0;
    TcpServerMessage servermsg;
    while (servermsg.fromByteArray(p + unpacklen,
            plen - unpacklen, &onepacklen))
    {
        unpacklen += onepacklen;
        //tcpserver->processor->processMessage(out);
        connctx->server->processMessage(bev, &servermsg, connctx);
        onepacklen = 0;
    }
    unpacklen += onepacklen;
    evbuffer_drain(buf, unpacklen);
}

void Tcpserver::connectcb(struct bufferevent *bev, short events, void *ctx)
{
    //ConnectInfo *info = (ConnectInfo *)ctx;
    ConnectionCtx *connctx = (ConnectionCtx *)ctx;
    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        ::writelog(InfoType, "Got an error %d (%s) on the connect to %s. events:%d",
                err,
                evutil_socket_error_to_string(err),
                connctx->ipport.c_str(),
                events);
        //bufferevent_free(bev);
        connctx->server->m_manager.removeConnection(connctx->ipport);
    }
    else if (events & (BEV_EVENT_CONNECTED))
    {
        ::writelog(InfoType, "connect ok.to %s", connctx->ipport.c_str());
        bufferevent_enable(bev, EV_READ | EV_WRITE);
        bufferevent_setcb(bev,
                          Tcpserver::readcb, 0,
                          Tcpserver::eventcb, connctx);
    }
    else
    {
        //
        assert(false);
    }
}

void Tcpserver::queuepoll(int fd, short events, void *ctx)
{
    Tcpserver *s = (Tcpserver *)ctx;
    s->processQueue();
}

void Tcpserver::heartBeatTimer(int fd, short events, void *ctx)
{
    Tcpserver *s = (Tcpserver *)ctx;
    s->m_manager.sendHeartBeat();
}

void Tcpserver::processMessage(bufferevent *bev, TcpServerMessage *servermsg, ConnectionCtx *ctx)
{
    if (servermsg->cmd == TcpServerMessage::SetConnServerId)
    {
        assert(servermsg->connServerId > 0);
        ctx->server->m_manager.setConnectionServer(servermsg->connServerId,
                                                   ctx->shared_from_this());
        // send a reply
        TcpServerMessage tsm;
        tsm.cmd = TcpServerMessage::SetConnServerId;
        tsm.connid = servermsg->connid;
        tsm.connServerId = servermsg->connServerId;
        std::vector<char> data = tsm.toByteArray();
        ctx->conn->send(data.data(), data.size());
        ::writelog(InfoType, "set conn server id %d.", servermsg->connServerId);
    }
    else if (servermsg->cmd == TcpServerMessage::NewMessage)
    {
        //
        processor->processMessage(servermsg);
#if 0
        ::writelog(InfoType, "get a messge from %d, connid: %lu",
                   servermsg->connServerId,
                   servermsg->connid);
#endif
    }
    else if (servermsg->cmd == TcpServerMessage::NewUdpMessage)
    {

    }
    else if (servermsg->cmd == TcpServerMessage::HeartBeat)
    {
        ::writelog(InfoType, "get heart beat from server: %d",
                   servermsg->connServerId);
    }
    else
    {
        assert(false);
    }
}

void Tcpserver::processQueue()
{
    std::list<WriteMessage*> writequeue;
    m_queuelock.lock();
    writequeue.swap(m_writequeue);
    m_queuelock.release();
    for (WriteMessage *msg : writequeue)
    {
        std::shared_ptr<ConnectionCtx> connctx = m_manager.getConnection(msg->ipport);
        if (!connctx)
        {
            // connect to server.
            //
            sockaddr_in sock;
            int len = sizeof(sock);
            memset(&sock, 0, sizeof(sock));
            if (evutil_parse_sockaddr_port(msg->ipport.c_str(),
                    (struct sockaddr *)&sock,
                    &len) == -1)
            {
                ::writelog(InfoType, "adresss error.%s\n", msg->ipport.c_str());
            }
            else
            {
                // connect to server and send data to .
                bufferevent *be = bufferevent_socket_new(m_evbase,
                    -1,
                    BEV_OPT_CLOSE_ON_FREE
                    | BEV_OPT_THREADSAFE);
                //
                std::shared_ptr<ConnectionCtx> connctx = std::make_shared<ConnectionCtx>();
                connctx->ipport = msg->ipport;
                connctx->server = this;
                connctx->conn = std::make_shared<Tcpconnection>(be);
                bufferevent_setcb(be,
                    0,//Tcpserver::readcb,
                    0, Tcpserver::connectcb, connctx.get());
                //bufferevent_enable(be, EV_READ | EV_WRITE);
                bufferevent_socket_connect(be, (struct sockaddr *)&sock, len);


                TcpServerMessage tsm;
                tsm.cmd = TcpServerMessage::NewMessage;
                tsm.clientmessage = msg->data.data();
                tsm.clientmessageLength = msg->data.size();
                std::vector<char> data = tsm.toByteArray();
                connctx->conn->send(data.data(), data.size());
                this->m_manager.insertConnection(msg->ipport, connctx);
            }
        }
        else
        {
            TcpServerMessage tsm;
            tsm.cmd = TcpServerMessage::NewMessage;
            tsm.clientmessage = msg->data.data();
            tsm.clientmessageLength = msg->data.size();
            std::vector<char> data = tsm.toByteArray();
            connctx->conn->send(data.data(), data.size());
        }
        delete msg;
    }
}

// static void writecb(struct bufferevent *bev, void *ctx);
void Tcpserver::eventcb(struct bufferevent *be, short events, void *ctx)
{
    ConnectionCtx *connctx = (ConnectionCtx *)ctx;
    if (events & BEV_EVENT_ERROR)
    {
        int err = EVUTIL_SOCKET_ERROR();
        ::writelog(InfoType,
                   "logical Got an error %d (%s) event:%d",
                   err,
                   evutil_socket_error_to_string(err),
                   events);
    }
    else if (events & BEV_EVENT_EOF)
    {
        ::writelog(InfoType,
                    "connectin closed: %d, %s",
                    connctx->conn->connectionServerId(),
                    connctx->ipport.c_str());

        connctx->server->m_manager.removeConnection(connctx->ipport);
        connctx->server->m_manager.removeConnection(connctx->conn->connectionServerId());
        // connectin closed

        //Tcpserver *s = (Tcpserver *)ctx;
        //std::string ipport;
        //auto it = s->m_beip.find(be);
        //if (it != s->m_beip.end())
        //{
        //    ipport = s->m_beip[be];
        //    s->m_manager.removeConnection(ipport);
        //    s->m_beip.erase(it);
        //    printf("connection closed: %s\n", ipport.c_str());
        //}
        //else
        //{
        //    assert(false);
        //}
    }
    else
    {
        // 
    }
}

std::shared_ptr<ConnectionCtx> Connectionmanager::getConnection(
    const std::string &ipport)
{
    //MutexGuard g(&m_connmutex);
    auto it = m_connection.find(ipport);
    if (it != m_connection.end())
    {
        return it->second;
    }
    else
    {
        return std::shared_ptr<ConnectionCtx>(0);
    }
}

std::shared_ptr<ConnectionCtx> Connectionmanager::getConnection(uint32_t serverid)
{
    auto it = m_serverIdMapconnection.find(serverid);
    if (it != m_serverIdMapconnection.end())
        return it->second;
    else
        return std::shared_ptr<ConnectionCtx>(0);
}

bool Connectionmanager::insertConnection(const std::string &ipport,
    const std::shared_ptr<ConnectionCtx> &conn)
{
    //MutexGuard g(&m_connmutex);

    auto it = m_connection.find(ipport);
    if (it != m_connection.end())
    {
        return false;
    }
    else
    {
        m_connection[ipport] = conn;
        return true;
    }
}

void Connectionmanager::setConnectionServer(uint32_t serverid,
                                            const std::shared_ptr<ConnectionCtx> &conn)
{
    conn->conn->setConnectionServerId(serverid);
    m_serverIdMapconnection.insert(std::make_pair(serverid, conn));
}

void Connectionmanager::removeConnection(const std::string &ipport)
{
    //MutexGuard g(&m_connmutex);

    auto it = m_connection.find(ipport);
    if (it != m_connection.end())
    {
        m_serverIdMapconnection.erase(it->second->conn->connectionServerId());
        m_connection.erase(it);
    }
}

void Connectionmanager::removeConnection(uint32_t serverid)
{
    auto it = m_serverIdMapconnection.find(serverid);
    if (it != m_serverIdMapconnection.end())
    {
        m_serverIdMapconnection.erase(it);
    }
}

void Connectionmanager::sendHeartBeat()
{
    TcpServerMessage tsm;
    tsm.cmd = TcpServerMessage::HeartBeat;
    std::vector<char> d = tsm.toByteArray();
    auto it = m_connection.begin();
    for (; it != m_connection.end(); ++it)
    {
        if (it->second->conn->connectionServerId() == 0)
        {
            it->second->conn->send(d.data(), d.size());
        }
    }
}


void Tcpconnection::send(const char *d, int len)
{
    //std::vector<char> out;
    //TcpDataPack::pack((char *)d, len, &out);
    if (bufferevent_write(bev, d, len) == -1)
    {
        ::writelog(ErrorType, "write error to %d", m_connectionServerId);
    }
}

uint32_t Tcpconnection::connectionServerId() const
{
    return m_connectionServerId;
}

void Tcpconnection::setConnectionServerId(const uint32_t &connectionServerId)
{
    m_connectionServerId = connectionServerId;
}
