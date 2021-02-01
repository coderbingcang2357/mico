#ifndef UTILSOCK_H__
#include <string>
struct sockaddr;
namespace Utilsock
{
std::string sockaddr2string(sockaddr *, int len);
}

#endif
