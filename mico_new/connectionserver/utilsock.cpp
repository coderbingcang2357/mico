#include <arpa/inet.h>
#include <sys/socket.h>
#include "netinet/in.h"
#include "utilsock.h"

std::string Utilsock::sockaddr2string(sockaddr *addr, int len)
{
    std::string res;
    if (addr->sa_family == AF_INET)
    {
        sockaddr_in *inaddr  = (sockaddr_in *)addr;
        char buf[100];
        inet_ntop(AF_INET, &(inaddr->sin_addr), buf, sizeof(buf));
        res.append(buf);
        res.append(":");
        res.append(std::to_string(inaddr->sin_port));
    }
    else if (addr->sa_family == AF_INET6)
    {
        sockaddr_in6 *inaddr  = (sockaddr_in6 *)addr;
        char buf[100];
        inet_ntop(AF_INET6, &(inaddr->sin6_addr), buf, sizeof(buf));
        res.append(buf);
        res.append(":");
        res.append(std::to_string(inaddr->sin6_port));
    }
    return res;
}
