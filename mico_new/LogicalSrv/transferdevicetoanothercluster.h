#ifndef __TRANSFERDEVICETOANOTHERCLUSTER__H
#define __TRANSFERDEVICETOANOTHERCLUSTER__H

#include <string>
#include <list>
#include <vector>
#include <stdint.h>
class Message;
class InternalMessage;
class UserOperator;
class ICache;
class Notification;

// 设备移交, 把设备从一个群移到另一个群
class TransferDeviceToAnotherCluster
{
public:
    TransferDeviceToAnotherCluster(UserOperator *uop);

    uint32_t deviceClusterChanged(uint64_t devid, uint64_t destclusterid, const std::string &destcluster,
                              std::list<InternalMessage *> *r);
    uint32_t req(const std::vector<uint64_t> &vDeviceID,
            uint64_t transferorID,
            uint64_t srcClusterID,
            uint64_t destClusterID, std::list<InternalMessage *> *r);
    uint32_t approval(uint8_t result,
                      const std::vector<uint64_t> &vDeviceID,
                 uint64_t transferID,
                 uint64_t destClusterOwner,
                 uint64_t srcClusterID,
                 uint64_t destClusterID,
                 std::list<InternalMessage *> *r);

private:
    void genNotifyDestClusterAdmin(
            uint64_t srcuserid,
            const std::string &srcusraccount,
            uint64_t destClusterSysAdminID,
            uint64_t srcClusterID,
            const std::string &srcClusterAccount,
            uint64_t destClusterID,
            const std::string &destClusterAccount,
            const std::vector<uint64_t> &vDeviceID,
            std::list<InternalMessage *> *r);

    // 通知原来的设备群设备被移走了
    void genNotifySrcClusterDeviceTrsanfered(Notification *,
                uint64_t srcClusterID,
                std::list<InternalMessage *> *r);
    // 通知目标群, 有设备移交过来了
    void genNotifyDestClusterDeviceTranfered(Notification *,
                uint64_t srcClusterID,
                std::list<InternalMessage *> *r);
    // 通知设备, 你被移到别的群了.
    void genNotifyDeviceBeTrsanferedToanotherCluster(
            const std::vector<uint64_t> &vDeviceID,
            const std::string &clusteraccount,
                std::list<InternalMessage *> *r);

    // 通知最初的移交者是否移交成功
    void genNotifyTrsanferResult(uint64_t transferID,
                uint8_t nofityAnscCode,
                uint8_t result,
                uint64_t srcClusterID,
                const std::string &srcClusteraccount,
                uint64_t destClusterID,
                const std::string &destClusterAccount,
                const std::vector<uint64_t> &vDeviceID,
                std::list<InternalMessage *> *r);

    // 所有的通知内容都是一样的
    void getNotify(uint8_t ansccode,
                uint64_t srcClusterID,
                const std::string &srcClusteraccount,
                uint64_t destClusterID,
                const std::string &destClusterAccount,
                const std::vector<uint64_t> &vDeviceID,
                Notification *out);

    //ICache *m_cache;
    UserOperator *m_useroperator;
};
#endif
