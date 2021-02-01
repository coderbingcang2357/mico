#ifndef DELETELOCALCACHE__H
#define DELETELOCALCACHE__H
#include "onlineinfo/icache.h"
#include "imessageprocessor.h"

class InternalMessage;

class DeleteLocalCache : public IMessageProcessor
{
    static const uint64_t Magic = 0xe7a2f7b3c9d2faff;
public:
    static InternalMessage *getDelCacheMessage(uint64_t userid, int targetserverid);

    DeleteLocalCache(ICache *localcache);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *localcache;
};

#endif
