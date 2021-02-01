#ifndef GENNOTIFYUSERONLINESTATUS__H
#define GENNOTIFYUSERONLINESTATUS__H

#include <stdint.h>
#include <list>

class InternalMessage;
class UserOperator;
class GenNotifyUserOnlineStatus
{
public:
    GenNotifyUserOnlineStatus(uint64_t userid, uint8_t onlinestatus, UserOperator *uop);

    void genNotify(std::list<InternalMessage *> *imsg);
private:
    void genClusterNotify(uint64_t clusterid, std::list<InternalMessage *> *imsg);

    uint64_t m_userid;
    uint64_t m_onlinestatu;

    UserOperator *m_useroperator;
};

#endif
