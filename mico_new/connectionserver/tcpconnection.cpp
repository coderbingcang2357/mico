#include <string.h>
#include "tcpconnection.h"
#include "util/logwriter.h"


TcpConnection::TcpConnection()
{

}

TcpConnection::~TcpConnection()
{
    if (be)
        bufferevent_free(be);
    ::writelog(InfoType, "~ close conn : %lu", this->connid);
}

void TcpConnection::send(char *d, int len)
{
    bufferevent_write(be, d, len);
}

void TcpConnection::setClientAddress(const sockaddrwrap &addr)
{
    this->clientaddr = addr;
}

sockaddrwrap &TcpConnection::getClientaddr()
{
    return clientaddr;
}
