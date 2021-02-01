#ifndef TRANSFERDEVICECLUSTER__H
#define TRANSFERDEVICECLUSTER__H

#include <set>
#include <functional>
#include "onlineinfo/icache.h"
#include "imessageprocessor.h"
#include "thread/mutexwrap.h"

// 移交设备群
class UserOperator;
class TransferDeviceCluster : public IMessageProcessor
{
    class TransferPair;
public:

    TransferDeviceCluster(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int req(Message *, std::list<InternalMessage *> *r);
    int verfy(Message *, std::list<InternalMessage *> *r);
    void genNotifyTageetUser(const TransferPair &tp, std::list<InternalMessage *> *r);
    void genNotifyTransferCluster(uint64_t oldowner,
        uint64_t newowner,
        uint64_t clusterid,
        uint8_t res,
        uint8_t issuccess,  // 数所库操作是否成功
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;

    class TransferPair
    {
    public:
        TransferPair(uint64_t u, uint64_t c, uint64_t t)
            : userid(u), clusterid(c), targetuserid(t)
        {
        }
        uint64_t userid;
        uint64_t clusterid;
        uint64_t targetuserid;
    };

    MutexWrap m_mutex;
    std::set<TransferPair, std::function<bool (const TransferPair &, const TransferPair &)> > m_transferset;
};
#endif
