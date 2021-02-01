#include <list>
#include "deletedevicefromcluster.h"
#include "useroperator.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "messageencrypt.h"

DeleteDeviceFromCluster::DeleteDeviceFromCluster(ICache *c, UserOperator *op)
    : m_cache(c), m_op(op)
{
}

int DeleteDeviceFromCluster::processMesage(Message *reqmsg,
                                           std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    char *c = reqmsg->content();
    uint32_t len = reqmsg->contentLen();
    if (len < sizeof(uint64_t) + sizeof(uint16_t))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    uint64_t adminid = reqmsg->objectID();
    uint64_t clusterid = ::ReadUint64(c);
    uint16_t count = ::ReadUint16(c + sizeof(clusterid));
    len = len - sizeof(clusterid);
    len = len - sizeof(count);
    c += sizeof(clusterid);
    c += sizeof(count);
    uint64_t devid = 0;
    if (len != sizeof(devid) * count)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }


    bool isadmin = false;
    if (m_op->checkClusterAdmin(adminid, clusterid, &isadmin) != 0
        || !isadmin)
    {
        ::writelog(InfoType, "del device check auth failed");
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }

    std::list<uint64_t> devidlist;
    for (int i = 0; i < count; ++i)
    {
        devid = ::ReadUint64(c + i * sizeof(devid));
        devidlist.push_back(devid);
    }
    if (m_op->deleteDeviceFromCluster(clusterid, devidlist) != 0)
    {
        ::writelog(InfoType, "del device exec sql failed");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    else
    {
        // ok
        msg->appendContent(ANSC::SUCCESS);
        // go through
    }
    //
    std::list<uint64_t> useridlist;
    m_op->getUserIDInDeviceCluster(clusterid, &useridlist);
    std::string clusteraccount;
    m_op->getClusterAccount(clusterid, &clusteraccount);
    std::string adminaccount;
    m_op->getUserAccount(adminid, &adminaccount);
    // gennotify
    this->genNotify(adminid, adminaccount,
                    clusterid, clusteraccount,
                    devidlist,
                    useridlist,
                    r);
    return r->size();

}

void DeleteDeviceFromCluster::genNotify(uint64_t adminid,
                                        const std::string &adminaccount,
                                        uint64_t clusterid,
                                        const std::string &clusteraccount,
                                        const std::list<uint64_t> &devids,
                                        const std::list<uint64_t> &userids,
                                        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterid); // 用户所在的群ID
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, adminid);
    ::WriteString(&buf, adminaccount);
    ::WriteUint16(&buf, uint16_t(devids.size()));

    printf("tag: ");
    for (size_t i = 0; i < buf.size(); ++i)
    {
        printf("%x ", uint8_t(buf[i]));
    }
    printf("\n");

    for (auto it = devids.begin(); it != devids.end(); ++it)
    {
        ::WriteUint64(&buf, *it);
    }

    Notification notify;
    notify.Init(0, // notify receiver
        ANSC::SUCCESS,
        CMD::NTP_DCL__REMOVE_DEVICES_FROM_DEVICE_CLUSTER,
        0, // serial number 在notify进程生成
        buf.size(),
        &buf[0]);

    std::vector<uint64_t> clusteridlist;
    //m_useroperator->getClusterIDListOfUser(m_userid, &clusteridlist);
    for (auto it = userids.begin(); it != userids.end(); ++it)
    {
        uint64_t userid = *it;
        notify.SetObjectID(userid); // notify receiver

        Message *msg = new Message;
        msg->setObjectID(userid);
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->appendContent(&notify);

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);
        r->push_back(imsg);
    }

}
