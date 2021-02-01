#ifndef UDP_SERVER_H__
#define UDP_SERVER_H__
#include <stdint.h>
#include <arpa/inet.h>
class Message;
class Udpserver
{
public:
    Udpserver();
    Message *recv();
    void send(Message *msg);

private:
    bool check(char *msgBuf, uint32_t msgLen);

    int m_fd;
};
#endif
