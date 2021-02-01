#ifndef UDP_SERVER_H__
#define UDP_SERVER_H__
#include <stdint.h>
#include <arpa/inet.h>
#include "serverqueue.h"
class Message;
class Udpserver
{
public:
    Udpserver(ServerQueue *masterqueue);
    ~Udpserver();
    void run();
    void quit();
    void addMessage(IConnectionServerMessage *msg);

private:
    void recv();
    bool check(char *msgBuf, uint32_t msgLen);
    void send(char *d, int dlen, sockaddr *addr, int addrlen);
    void processQueue();

    int m_fd;
    bool m_isrun = true;
    ServerQueue *m_masterqueue;
    ServerQueue m_queue;
};
#endif
