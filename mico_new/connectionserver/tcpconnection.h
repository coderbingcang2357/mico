#ifndef TCPCONNECTION__H__
#define TCPCONNECTION__H__
#include <event2/bufferevent.h>
#include <stdint.h>
#include "util/sock2string.h"
//#include <netinet/in.h>
class TcpConnection
{
public:
    TcpConnection();
    ~TcpConnection();
    enum ConnectState {
        Invalid,
        Connecting,
        Conneccted,
        Valid
    };
    enum ConnType {
        Client,
        Server
    };
    void send(char *d, int len);
    void setClientAddress(const sockaddrwrap &addr);

    uint64_t connid = 0;
    uint32_t logicalserverid = 0;
    ConnectState state = Invalid;
    ConnType ct = Client;
    bufferevent *be = nullptr;

    sockaddrwrap &getClientaddr();

private:
    sockaddrwrap clientaddr;
};
#endif
