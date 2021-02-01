#ifndef TCP_SERVER__H
#define TCP_SERVER__H

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <list>
#include "thread/threadwrap.h"
#include "tcpmessageprocessor.h"
#include "util/tcpdatapack.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
#include "Message/tcpservermessage.h"
#include "util/sock2string.h"

class Tcpconnection
{
public:
    Tcpconnection(bufferevent *e) : bev(e) {}
    ~Tcpconnection()
    {
        printf("connection closed.\n");
        bufferevent_free(bev);
    }

    void send(const char *d, int len);
    uint32_t connectionServerId() const;
    void setConnectionServerId(const uint32_t &connectionServerId);

private:
    struct bufferevent *bev;
    uint32_t m_connectionServerId = 0;
};

class Tcpserver;
class ConnectionCtx : public std::enable_shared_from_this<ConnectionCtx>
{
public:
    Tcpserver *server;
    std::shared_ptr<Tcpconnection> conn;
    std::string ipport; // only used when connect to server
};

class Connectionmanager
{
public:
    std::shared_ptr<ConnectionCtx> getConnection(const std::string &ipport);
    std::shared_ptr<ConnectionCtx> getConnection(uint32_t serverid);
    bool insertConnection(const std::string &ipport, const std::shared_ptr<ConnectionCtx> &conn);
    void setConnectionServer(uint32_t serverid, const std::shared_ptr<ConnectionCtx> &conn);
    void removeConnection(const std::string &ipport);
    void removeConnection(uint32_t serverid);
    void sendHeartBeat();

private:
    std::map<std::string, std::shared_ptr<ConnectionCtx> > m_connection;
    // serverid - connection
    std::map<uint32_t, std::shared_ptr<ConnectionCtx> > m_serverIdMapconnection;
};

class WriteMessage
{
public:
    std::string ipport;
    std::vector<char> data;
};

class Tcpserver
{
public:
    Tcpserver(const std::string &ip, uint16_t port, Tcpmessageprocessor *);
    ~Tcpserver();
    void create();
    void send(const std::string &ipport, const std::vector<char> &data);
    void send(uint32_t connserverid, uint64_t connid, const sockaddrwrap &addr, char *p, int len);

private:
    void run();
    void initlog();
    static void writelogcb(int severity, const char *msg);
    static void acceptcb(evconnlistener *,
        evutil_socket_t fd, struct sockaddr *addr, int len, void *ctx);
    static void listererErrorcb(evconnlistener *el, void *ctx);
    static void readcb(struct bufferevent *bev, void *ctx);
    static void writecb(struct bufferevent *bev, void *ctx);
    static void eventcb(struct bufferevent *bev, short events, void *ctx);
    static void connectcb(struct bufferevent *bev, short events, void *ctx);
    static void queuepoll(int fd, short events, void *ctx);
    static void heartBeatTimer(int fd, short events, void *ctx);

    void processMessage(struct bufferevent *bev, TcpServerMessage *servermsg, ConnectionCtx *ctx);
    void processQueue();

    std::string m_ip;
    uint16_t m_port;
    Thread *m_t;
    struct event_base *m_evbase;
    struct evconnlistener *m_listener;
    friend class ServerThread;

    //std::map<struct bufferevent *, std::string> m_beip;
    Connectionmanager m_manager;
    Tcpmessageprocessor *processor;

    MutexWrap m_queuelock;
    std::list<WriteMessage *> m_writequeue;
};

class ConnectInfo
{
public:
    Tcpserver *s;
    std::vector<char> data;
    std::string ipport;
};
#endif
