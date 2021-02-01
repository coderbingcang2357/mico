#ifndef SERVERSLAVE_H__
#define SERVERSLAVE_H__
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <assert.h>
#include <vector>
#include <string>
#include <stdint.h>
#include <map>
#include <memory>
#include "connectionservermessage.h"
#include "Message/tcpservermessage.h"
#include "tcpclientmessage.h"
#include "serverqueue.h"
#include "tcpconnection.h"
#include "timer/wheeltimer.h"

struct sockaddr;
class ServerSlave;

class SlaveContext
{
public:
    ServerSlave *server = nullptr;
    std::shared_ptr<TcpConnection> conn;
    std::weak_ptr<WheelTimer> timeoutTimer;
};

class ServerSlave
{
public:
    ServerSlave(ServerQueue *masterqueue);
    ~ServerSlave();

    static void queuepoll(int fd, short what, void *ctx);
    static void checkConnTimeout(int fd, short what, void *ctx);
    static void readcb(struct bufferevent *bev, void *ctx);
    static void eventcb(struct bufferevent *bev, short events, void *ctx);

    void addMessage(IConnectionServerMessage *msg);
    void run();

private:
    void processMessage();
    void checkConnTimeoutHelp();
    void newConnection(NewConnectionMessage *ncm);
    void sendToClient(SendToClientMessage *scm);
    void setConnectionId(uint64_t connid, const TcpClientMessage &tcm);
    void connectionTimeout(std::weak_ptr<SlaveContext> weaksc);

    event_base *m_eventbase;
    ServerQueue m_queue;
    ServerQueue *m_masterqueue;

    std::map<uint64_t, std::shared_ptr<SlaveContext>> m_connections;
    WheelTimerPoller *m_connectionTimeoutRollTimer;
};
#endif
