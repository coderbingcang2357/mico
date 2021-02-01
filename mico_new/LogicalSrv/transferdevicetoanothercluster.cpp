#include <memory>
#include "transferdevicetoanothercluster.h"
#include "useroperator.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "sceneConnectDevicesOperator.h"
#include "useroperator.h"
#include "dbaccess/cluster/clusterdal.h"
//#include

TransferDeviceToAnotherCluster::TransferDeviceToAnotherCluster(UserOperator *uop)
    : m_useroperator(uop)
{
}

uint32_t TransferDeviceToAnotherCluster::deviceClusterChanged(uint64_t devid, uint64_t destclusterid, const std::string &destcluster, std::list<InternalMessage *> *r)
{
    // 已经在群里面了,返回成功
    std::vector<uint64_t> vDeviceID{devid};
    if (m_useroperator->isDeviceInCluster(vDeviceID, destclusterid))
    {
        return ANSC::SUCCESS;
        //msg->appendContent(ANSC::Failed);
        //return r->size();
    }

    // TODO .源群直接从数据库中取
    DeviceInfo *deviceInfo;
    deviceInfo = m_useroperator->getDeviceInfo(devid);
    std::unique_ptr<DeviceInfo> del_(deviceInfo);
    uint64_t srcclusterid = deviceInfo->clusterID;
    bool hasSrcClusterid = true;;
    std::string srccluster;
    if (m_useroperator->getClusterAccount(srcclusterid, &srccluster) < 0)
    {
        ::writelog("transfer device get src clustter account failed.");
        hasSrcClusterid = false;
        //return ANSC::FAILED;
        //msg->appendContent(ANSC::Failed);
        //return r->size();;
    }

    Notification notify;

    if (hasSrcClusterid)
    {
        getNotify(ANSC::SUCCESS,
                  srcclusterid, //srcClusterID,
                srccluster, // srcClusteraccount,
                destclusterid, // destClusterID,
                destcluster, // destClusterAccount,
                vDeviceID,
                &notify);
    }

    // 检是否达到最大成员限制
    ClusterDal cd;
    if (!cd.canAddDevice(destclusterid, vDeviceID.size()))
    {
        ::writelog(InfoType, "transfer device max limit: %s->%s",
                srccluster.c_str(), destcluster.c_str());
        return ANSC::CLUSTER_MAX_DEVICE_LIMIT;
    }

    if(m_useroperator->modifyDevicesOwnership(vDeviceID, srcclusterid, destclusterid) < 0)
    {
        ::writelog("transfer device db operator failed.");
        //msg->appendContent(&ANSC::Failed, Message::SIZE_ANSWER_CODE);
        //return r->size();
        return ANSC::FAILED;
    }
    // 通知原来的设备群设备被移走了
    if (hasSrcClusterid)
    {
        genNotifySrcClusterDeviceTrsanfered(&notify,
            srcclusterid, // srcClusterID,
            r);
    }

    // 通知目标群, 有设备移交过来了
    genNotifyDestClusterDeviceTranfered(&notify,
        destclusterid, // destClusterID,
        r);

    // 设备被移交后那么所有这个设备的连接都不可用, 所以要删除
    SceneConnectDevicesOperator sceneop(m_useroperator);
    sceneop.removeConnectorOfDevice(vDeviceID);

    //
    return ANSC::SUCCESS;
}

uint32_t TransferDeviceToAnotherCluster::req(const std::vector<uint64_t> &vDeviceID,
                                        uint64_t transferorID,
                                        uint64_t srcClusterID,
                                        uint64_t destClusterID,
                                        std::list<InternalMessage *> *r)
{

    if (m_useroperator->isDeviceInCluster(vDeviceID, destClusterID))
    {
        ::writelog("device already in cluster");
        //msg->appendContent(ANSC::Failed);
        return ANSC::FAILED;
        //return r->size();
    }

    bool isAdmin = m_useroperator->getDeviceClusterOwner(srcClusterID)==transferorID && transferorID > 0;
    if(!isAdmin)
    {
        ::writelog("transfer device no auth.");
        //msg->appendContent(&ANSC::AuthErr, Message::SIZE_ANSWER_CODE);
        return ANSC::AUTH_ERROR;
        //return r->size();
    }

    std::string transferorAccount; // 用户名帐号
    if(m_useroperator->getUserAccount(transferorID, &transferorAccount) < 0)
    {
        ::writelog("transfer device get user accout failed.");
        return ANSC::FAILED;
        //msg->appendContent(&ANSC::Failed, Message::SIZE_ANSWER_CODE);
        //return r->size();
    }

    string srcClusterAccount;// 用户群名
    if(m_useroperator->getClusterAccount(srcClusterID, &srcClusterAccount) < 0)
    {
        ::writelog("transfer device get cluster accout failed.");
        return ANSC::FAILED;
        //msg->appendContent(&ANSC::Failed, Message::SIZE_ANSWER_CODE);
        //return r->size();
    }

    string destClusterAccount; //...
    if(m_useroperator->getClusterAccount(destClusterID, &destClusterAccount) < 0)
    {
        ::writelog("transfer device get dest cluster accout failed.");
        return ANSC::FAILED;
        //msg->appendContent(&ANSC::Failed, Message::SIZE_ANSWER_CODE);
        //return r->size();
    }

    // 群的所有者
    uint64_t destClusterSysAdminID;
    if(m_useroperator->getSysAdminID(destClusterID, &destClusterSysAdminID) < 0)
    {
        ::writelog(InfoType, "transfer device get dest cluster sys admin failed.  %lu", destClusterID);
        return ANSC::FAILED;
        //return r->size();
    }

    genNotifyDestClusterAdmin(
        transferorID,
        transferorAccount,
        destClusterSysAdminID,
        srcClusterID,
        srcClusterAccount,
        destClusterID,
        destClusterAccount,
        vDeviceID,
        r);
    return ANSC::SUCCESS;
}

uint32_t TransferDeviceToAnotherCluster::approval(uint8_t result, const std::vector<uint64_t> &vDeviceID,
                                             uint64_t transferID,
                                             uint64_t destClusterOwner,
                                             uint64_t srcClusterID,
                                             uint64_t destClusterID,
                                             std::list<InternalMessage *> *r)
{
    uint8_t notifyAnscCode = ANSC::SUCCESS;

    //--------------------------------------------------------
    std::string srcClusteraccount;
    std::string destClusterAccount;

    if (m_useroperator->getClusterAccount(srcClusterID, &srcClusteraccount) != 0
        || m_useroperator->getClusterAccount(destClusterID, &destClusterAccount) != 0)
    {
        //msg->appendContent(&ANSC::Failed, Message::SIZE_ANSWER_CODE);
        //return r->size();
        notifyAnscCode = ANSC::CLUSTER_ID_OR_DEVICE_ID_ERROR;
        // 通知最初的移交者是否移交成功
        genNotifyTrsanferResult(transferID,
            notifyAnscCode,
            ANSC::ACCEPT,
            srcClusterID,
            srcClusteraccount,
            destClusterID,
            destClusterAccount,
            vDeviceID,
            r);
        return notifyAnscCode;
    }

    if(result == ANSC::ACCEPT)
    {
        // check auth
        bool isAdmin = m_useroperator->getDeviceClusterOwner(destClusterID) == destClusterOwner
                        && destClusterID > 0;
        if(!isAdmin)
        {
            ::writelog("transfer device dest user no auth.");
            //msg->appendContent(&ANSC::AuthErr, Message::SIZE_ANSWER_CODE);
            notifyAnscCode = ANSC::AUTH_ERROR;
            // 通知最初的移交者是否移交成功
            genNotifyTrsanferResult(transferID,
                notifyAnscCode,
                ANSC::ACCEPT,
                srcClusterID,
                srcClusteraccount,
                destClusterID,
                destClusterAccount,
                vDeviceID,
                r);
            return notifyAnscCode;
        }



        // 检是否达到最大成员限制
        ClusterDal cd;
        if (!cd.canAddDevice(destClusterID, vDeviceID.size()))
        {
            ::writelog(InfoType, "transfer device max limit: %s->%s",
                    srcClusteraccount.c_str(), destClusterAccount.c_str());
            notifyAnscCode = ANSC::CLUSTER_MAX_DEVICE_LIMIT;
            // 通知最初的移交者是否移交成功
            genNotifyTrsanferResult(transferID,
                notifyAnscCode,
                ANSC::ACCEPT,
                srcClusterID,
                srcClusteraccount,
                destClusterID,
                destClusterAccount,
                vDeviceID,
                r);
            return notifyAnscCode;
        }

        std::list<uint64_t> ldeviceID;
        for (uint32_t i = 0; i < vDeviceID.size(); ++i)
        {
            ldeviceID.push_back(vDeviceID[i]);
        }
        if (!m_useroperator->isAllDeviceInCluster(srcClusterID, ldeviceID))
        {
            notifyAnscCode = ANSC::DEVICE_ID_ERROR;
            // 通知最初的移交者是否移交成功
            genNotifyTrsanferResult(transferID,
                notifyAnscCode,
                ANSC::ACCEPT,
                srcClusterID,
                srcClusteraccount,
                destClusterID,
                destClusterAccount,
                vDeviceID,
                r);
            return notifyAnscCode;
        }

        //
        if(m_useroperator->modifyDevicesOwnership(vDeviceID, srcClusterID, destClusterID) < 0)
        {
            ::writelog("transfer device db operator failed.");
            notifyAnscCode = ANSC::FAILED;
            // 通知最初的移交者是否移交成功
            genNotifyTrsanferResult(transferID,
                notifyAnscCode,
                ANSC::ACCEPT,
                srcClusterID,
                srcClusteraccount,
                destClusterID,
                destClusterAccount,
                vDeviceID,
                r);
            return notifyAnscCode;
        }

        notifyAnscCode = ANSC::SUCCESS;
        Notification notify;
        getNotify(notifyAnscCode,
                srcClusterID,
                srcClusteraccount,
                destClusterID,
                destClusterAccount,
                vDeviceID,
                &notify);

        // ok
        // 通知原来的设备群设备被移走了
        genNotifySrcClusterDeviceTrsanfered(&notify,
            srcClusterID,
            r);

        // 通知目标群, 有设备移交过来了
        genNotifyDestClusterDeviceTranfered(&notify,
            destClusterID,
            r);
        // 通知设备, 你被移到别的群了.
        genNotifyDeviceBeTrsanferedToanotherCluster(vDeviceID,
            destClusterAccount,
            r);

        // 设备被移交后那么所有这个设备的连接都不可用, 所以要删除
        SceneConnectDevicesOperator sceneop(m_useroperator);
        sceneop.removeConnectorOfDevice(vDeviceID);

        //// 给设备发通知? 群号改了..
        //uint32_t notifNumber = 0;
        //uint8_t prefix = 0xFF;
        //uint8_t opcode = 0x04;
        //uint8_t clusterAccountLen = destClusterAccount.length();
        //for(vector<uint64_t>::iterator iter = vDeviceID.begin();
        //    iter != vDeviceID.end();
        //    ++iter)
        //{
        //    Message* deviceMsg = new Message();
        //    deviceMsg->SetPrefix(DEVICE_MSG_PREFIX);
        //    deviceMsg->SetObjectID(*iter);
        //    deviceMsg->SetCommandID(CMD::CMD_GEN__NOTIFY);
        //    deviceMsg->appendContent(&ANSC::Success, Message::SIZE_ANSWER_CODE);
        //    deviceMsg->appendContent(&notifNumber, Message::SIZE_NOTIF_NUMBER);
        //    // 通知类型, 在LogicalSrv针对这个有特殊处理, 因为消息格式有些特别, 所以要专门修改消息格式
        //    deviceMsg->appendContent(&CMD::DNTP__TRANSFER_DEIVCES, Message::SIZE_NOTIF_TYPE);
        //    deviceMsg->appendContent(&prefix, sizeof(prefix));
        //    deviceMsg->appendContent(&opcode, sizeof(opcode));
        //    deviceMsg->appendContent(&clusterAccountLen, sizeof(clusterAccountLen));
        //    deviceMsg->appendContent(destClusterAccount.c_str(), clusterAccountLen);
        //    deviceMsg->SetSuffix(DEVICE_MSG_SUFFIX);

        //    InternalMessage *imsg = new InternalMessage(deviceMsg);
        //    r->push_back(imsg);
        //}

        // 通知最初的移交者是否移交成功
        genNotifyTrsanferResult(transferID,
            ANSC::SUCCESS,
            ANSC::ACCEPT,
            srcClusterID,
            srcClusteraccount,
            destClusterID,
            destClusterAccount,
            vDeviceID,
            r);

        return ANSC::SUCCESS;
    }
    else
    {
        // 通知最初的移交者是否移交成功
        genNotifyTrsanferResult(transferID,
            ANSC::SUCCESS,
            ANSC::REFUSE,
            srcClusterID,
            srcClusteraccount,
            destClusterID,
            destClusterAccount,
            vDeviceID,
            r);
        return ANSC::SUCCESS;
    }
}

void TransferDeviceToAnotherCluster::genNotifyDestClusterAdmin(
            uint64_t srcUserid,
            const std::string &srcuseraccount,
            uint64_t destClusterSysAdminID,
            uint64_t srcClusterID,
            const std::string &srcClusterAccount,
            uint64_t destClusterID,
            const std::string &destClusterAccount,
            const std::vector<uint64_t> &vDeviceID,
            std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, srcUserid);
    ::WriteString(&buf, srcuseraccount);
    ::WriteUint64(&buf, srcClusterID);
    ::WriteString(&buf, srcClusterAccount);
    ::WriteUint64(&buf, destClusterID);
    ::WriteString(&buf, destClusterAccount);
    ::WriteUint16(&buf, uint16_t(vDeviceID.size()));
    for (auto it = vDeviceID.begin(); it != vDeviceID.end(); ++it)
    {
        std::string devname;
        if (m_useroperator->getDeviceName(*it, &devname) != 0)
            continue;
        ::WriteUint64(&buf, *it);
        // 设备名
        ::WriteString(&buf, devname);
    }

    Notification notify;
    notify.Init(destClusterSysAdminID,
        ANSC::SUCCESS,
        CMD::NTP_DCL__TRANSFER_DEVICES__REQ,
        0, // sn
        buf.size(),
        &buf[0]);
    Message *msg = new Message;
    msg->setObjectID(destClusterSysAdminID);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}

// 通知原来的设备群设备被移走了
void TransferDeviceToAnotherCluster::genNotifySrcClusterDeviceTrsanfered(
                Notification *notify,
                uint64_t srcClusterID,
                std::list<InternalMessage *> *r)
{
    notify->setNotifyType(CMD::DCL__TRANSFER_DEVICES__NOTIFY_SRC_CLUSTER);
    std::list<uint64_t> idlist;
    if (m_useroperator->getUserIDInDeviceCluster(srcClusterID, &idlist) != 0)
        return;
    for (auto it = idlist.begin(); it != idlist.end(); ++it)
    {
        notify->SetObjectID(*it);
        Message *msg = new Message;
        msg->setObjectID(*it);
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->appendContent(notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);
        r->push_back(imsg);
    }
}

// 通知目标群, 有设备移交过来了
void TransferDeviceToAnotherCluster::genNotifyDestClusterDeviceTranfered(
        Notification *notify,
        uint64_t destClusterID,
        std::list<InternalMessage *> *r)
{
    notify->setNotifyType(CMD::DCL__TRANSFER_DEVICES__NOTIFY_DEST_CLUSTER);
    std::list<uint64_t> idlist;
    if (m_useroperator->getUserIDInDeviceCluster(destClusterID, &idlist) != 0)
        return;
    for (auto it = idlist.begin(); it != idlist.end(); ++it)
    {
        notify->SetObjectID(*it);
        Message *msg = new Message;
        msg->setObjectID(*it);
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->appendContent(notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);
        r->push_back(imsg);
    }
}

// 通知设备, 你被移到别的群了.
void TransferDeviceToAnotherCluster::genNotifyDeviceBeTrsanferedToanotherCluster(
            const std::vector<uint64_t> &vDeviceID,
            const std::string &clusteraccount,
            std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint8(&buf, uint8_t(0xff));
    ::WriteUint8(&buf, uint8_t(0x04));
    ::WriteUint8(&buf, uint8_t(clusteraccount.size()));
    char *p =(char *)clusteraccount.c_str();
    buf.insert(buf.end(), p, p + uint8_t(clusteraccount.size()));
    ::WriteUint64(&buf, uint64_t(0)); // 通知序列号, 在别的地地方生成
    ::WriteUint16(&buf, uint16_t(CMD::DNTP__TRANSFER_DEIVCES)); // 通知号

    Notification notify;
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::DNTP__TRANSFER_DEIVCES,
        0,
        buf.size(),
        &buf[0]);

    for (auto it = vDeviceID.begin(); it != vDeviceID.end(); ++it)
    {
        notify.SetObjectID(*it);
        Message *msg = new Message;
        // msg->SetCommandID(CMD::CMD_NewNotify); // CMD_NewSessionalNotify
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->setObjectID(*it);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);
        r->push_back(imsg);
    }
}

// 通知最初的移交者是否移交成功
void TransferDeviceToAnotherCluster::genNotifyTrsanferResult(
                uint64_t transferID,
                uint8_t nofityAnscCode,
                uint8_t result,
                uint64_t srcClusterID,
                const std::string &srcClusteraccount,
                uint64_t destClusterID,
                const std::string &destClusterAccount,
                const std::vector<uint64_t> &vDeviceID,
                std::list<InternalMessage *> *r)
{
    Notification notify;
    std::vector<char> buf;
    ::WriteUint8(&buf, result); // 是否同意
    ::WriteUint64(&buf, srcClusterID);
    ::WriteString(&buf, srcClusteraccount);
    ::WriteUint64(&buf, destClusterID);
    ::WriteString(&buf, destClusterAccount);
    ::WriteUint16(&buf, uint16_t(vDeviceID.size()));
    for (auto it = vDeviceID.begin(); it != vDeviceID.end(); ++it)
    {
        std::string devname;
        if (m_useroperator->getDeviceName(*it, &devname) != 0)
            continue;
        ::WriteUint64(&buf, *it);
        ::WriteString(&buf, devname);
    }

    notify.Init(transferID, // 接收者
            nofityAnscCode,
            CMD::NTP_DCL__TRANSFER_DEVICES__RESULT,
            0, // 序列号
            buf.size(),
            &buf[0]);

    Message *msg = new Message;
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->setObjectID(transferID);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}

// 所有的通知内容都是一样的
void TransferDeviceToAnotherCluster::getNotify(uint8_t ansccode,
                                               uint64_t srcClusterID,
            const std::string &srcClusteraccount,
            uint64_t destClusterID,
            const std::string &destClusterAccount,
            const std::vector<uint64_t> &vDeviceID,
            Notification *out)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, srcClusterID);
    ::WriteString(&buf, srcClusteraccount);
    ::WriteUint64(&buf, destClusterID);
    ::WriteString(&buf, destClusterAccount);
    ::WriteUint16(&buf, uint16_t(vDeviceID.size()));
    for (auto it = vDeviceID.begin(); it != vDeviceID.end(); ++it)
    {
        ::WriteUint64(&buf, *it);
    }
    out->Init(0, // 接收者, 在后面改
            ansccode,
            CMD::DCL__TRANSFER_DEVICES__NOTIFY_SRC_CLUSTER,
            0, // 序列号
            buf.size(),
            &buf[0]);
}

