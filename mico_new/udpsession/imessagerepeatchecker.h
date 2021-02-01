#ifndef iMessageRepeatChecker__h
#define iMessageRepeatChecker__h
#include <stdint.h>

struct sockaddr;
class IMessageRepeatChecker
{
public:
    virtual ~IMessageRepeatChecker() {}
    virtual bool isMessageRepeat(sockaddr *addr,
                    uint64_t clientid,
                    uint16_t msgSerialNumber,
                    uint16_t commandid) = 0;
    virtual void createSession(sockaddr *addr, uint64_t clientid) = 0;
    virtual void removeSession(sockaddr *addr, uint64_t clientid) = 0;
};
#endif
