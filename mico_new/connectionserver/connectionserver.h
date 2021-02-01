#ifndef CONNECTIONSERVER_H_
#define CONNECTIONSERVER_H_
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/thread.h>
#include <assert.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <map>
#include <memory>
#include <set>
#include "tcpconnection.h"
#include "serverslave.h"
#include "connectionservermessage.h"
#include "serverqueue.h"
#include "logicalservers.h"
#include "thread/threadwrap.h"
#include "./udpserver.h"

class ConnectionServer;
class ConnCtx
{
public:
    ConnectionServer *server = nullptr;
    std::shared_ptr<TcpConnection> conn;
};

class ConnectionServer
{
public:
    ConnectionServer(const std::string &bindaddr, uint16_t port, int threadcnt);
    ~ConnectionServer();
    int run();
    void setLogicalservers(const LogicalServers &logicalservers);

private:
    static void writelogcb(int severity, const char *msg);
    static void acceptcb(evconnlistener *listener,
        evutil_socket_t fd, struct sockaddr *addr, int len, void *ctx);
    static void listererErrorcb(evconnlistener *listener, void *ctx);
    static void readcb(struct bufferevent *bev, void *ctx);
    //static void writecb(struct bufferevent *bev, void *ctx);
    static void eventcb(struct bufferevent *bev, short events, void *ctx);
    static void connectcb(struct bufferevent *bev, short events, void *ctx);
    static void queuepoll(evutil_socket_t fd, short what, void *ctx);
    static void connectToLogicalServerPoll(evutil_socket_t fd, short what, void *ctx);
    static void connectToLogicalServerAtStart(evutil_socket_t fd, short what, void *ctx);
    static void heartbeatTimer(evutil_socket_t fd, short what, void *ctx);

private:
    //void insertConnection(uint64_t cid, const std::shared_ptr<TcpConnection> &c);
    //std::shared_ptr<TcpConnection> getConnection(uint64_t id);
    //std::shared_ptr<TcpConnection> getLogicalServerConnection();
    //void removeConnection(uint64_t id);
    void createListener();

    int getLogicalServerId();
    ServerSlave *allocSlave();
    ServerSlave *findConnectionSlave(uint64_t connid);

    void processQueueMessage();

    void connectToLogicalServer();
    void setConnectionServerId(ConnCtx *connctx);
    void connectToLogicalServerPollHelp();
    void sendHeartBeat();

    event_base *m_eventbase;
    std::string m_bindaddr;
    uint16_t m_port;
    evconnlistener *m_listener = nullptr;
    // ID -> tcpconn
    //std::map<uint64_t, std::shared_ptr<TcpConnection>> m_connections;
    ConnCtx *m_listenctx = nullptr;
    // serverid - connection
    std::map<int32_t, std::shared_ptr<ConnCtx>> m_logicalServerConnections;

    // connid - ServerSlave
    std::map<uint64_t, ServerSlave *> m_connidMapSlave;
    std::list<ServerSlave*> m_slaves;

    ServerQueue m_queue;
    LogicalServers m_logicalservers;

    std::list<int32_t> m_logicalserid;

    int m_threadcnt = 2;

    std::list<Thread*> m_slavethread;

    // udp
    Udpserver m_udpserver;
    Thread *m_udpthread;
    // udpaddr - serverid
    std::map<std::string, uint32_t> m_udpconnection;
};
#endif
