#ifndef __PUSHSERVICE__H
#define __PUSHSERVICE__H
#include <list>
#include "pushmessage.h"
class IMysqlConnPool;
class InternalMessage;
class PushService
{
public:
    PushService(IMysqlConnPool *pool);
    std::list<InternalMessage*> get(uint64_t userid);
    void messageack(uint64_t userid, uint64_t msgid);
    std::list<InternalMessage*> getPushMessage(const std::list<uint64_t> &receivers, uint64_t msgid);

private:
    InternalMessage *pushMessaeToIntermessage(uint64_t userid, const PushMessage &pushmessage);
    PushMessageDatabase m_pushmessagedatabase;
};
#endif

