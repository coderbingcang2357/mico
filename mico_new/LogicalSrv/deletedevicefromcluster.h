#ifndef DELETEDEVICEFROMCLUSTER__H
#define DELETEDEVICEFROMCLUSTER__H
#include <list>
#include <stdint.h>
#include <string>
#include "imessageprocessor.h"

class ICache;
class UserOperator;
class Message;
class DeleteDeviceFromCluster : public IMessageProcessor
{
public:
    DeleteDeviceFromCluster(ICache *c, UserOperator *op);
    int processMesage(Message *reqmsg, std::list<InternalMessage *> *r);

private:
    void genNotify(uint64_t adminid, const std::string &adminaccount,
                   uint64_t clusterid, const std::string &clusteraccount,
                   const std::list<uint64_t> &devids,
                   const std::list<uint64_t> &userids,
                   std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_op;
};
#endif
