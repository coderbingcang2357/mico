#ifndef ICLIENT__H
#define ICLIENT__H
#include <vector>
#include <string>
#include <time.h>
#include <netinet/in.h>
#include "util/sock2string.h"

class IClient
{
public:
    virtual ~IClient() {}
    virtual uint64_t id() = 0;
    virtual std::vector<char> sessionKey() = 0;
    virtual uint32_t loginServerID() = 0;
    virtual sockaddrwrap &sockAddr() = 0;
    virtual std::string name() = 0;
    virtual time_t loginTime() = 0;
    virtual int connectionServerId() { return 0; }
    virtual uint64_t connectionId() { return 0; }
};
#endif
