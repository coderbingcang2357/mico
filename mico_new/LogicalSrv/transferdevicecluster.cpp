#include "useroperator.h"
#include "transferdevicecluster.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "Crypt/tea.h"
#include "util/util.h"
#include "messageencrypt.h"
#include "util/logwriter.h"
#include "thread/mutexguard.h"
#include "dbaccess/user/userdao.h"

TransferDeviceCluster::TransferDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop),
    m_transferset([](const TransferPair &l,const TransferPair &r)
    {
        if (l.userid < r.userid)
        {
            return true;
        }
        else if (l.userid == r.userid)
        {
            if (l.clusterid < r.clusterid)
                return true;
            else
                return false;
        }
        else
        {
            return false;
        }
    })
{

}

int TransferDeviceCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if (::decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("transfer device cluster decrypt failed.");
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(ANSC::MESSAGE_ERROR);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(true);
        r->push_back(imsg);
        return r->size();
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__TRANSFER_DEVCLUSTER_REQ)
    {
        return req(reqmsg, r);
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__TRANSFER_DEVCLUSTER_VERIRY)
    {
        return verfy(reqmsg, r);
    }
    else
    {
        assert(false);
    }
    return 0;
}

int TransferDeviceCluster::req(Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint64_t userid = reqmsg->objectID();
    uint64_t clusterid;
    uint64_t targetuserid;

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if (plen < sizeof(clusterid) + sizeof(targetuserid))
    {
        ::writelog("transfer cluster error message");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    clusterid = ::ReadUint64(p);
    p += sizeof(clusterid);
    plen -= sizeof(clusterid);

    targetuserid = ::ReadUint64(p);
    p += sizeof(targetuserid);
    plen -= sizeof(targetuserid);

    uint64_t sysid;
    if (m_useroperator->getSysAdminID(clusterid, &sysid) != 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    if (sysid != userid)
    {
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }

    TransferPair tp(userid, clusterid, targetuserid);

    // lock
    m_mutex.lock();
    auto it = m_transferset.find(tp);
    if (it != m_transferset.end())
    {
        TransferPair &tmp = const_cast<TransferPair &>(*it);
        tmp.targetuserid = tp.targetuserid;
    }
    else
    {
        m_transferset.insert(tp);
    }
    // release
    m_mutex.release();

    msg->appendContent(ANSC::SUCCESS);

    genNotifyTageetUser(tp, r);

    return r->size();
}

int TransferDeviceCluster::verfy(Message *reqmsg, std::list<InternalMessage *> *r)
{
    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    uint8_t res;
    uint64_t clusterid;
    uint64_t userid;
    if (plen < sizeof(res) + sizeof(clusterid) + sizeof(userid))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    res = ReadUint8(p);
    p += sizeof(res);

    clusterid = ReadUint64(p);
    p += sizeof(clusterid);

    userid = ReadUint64(p);
    p += sizeof(userid);

    TransferPair tmptp(userid, clusterid, reqmsg->objectID());

    // 查找此移交验证是否存在
    bool isfind = false;
    uint64_t targetuserid = 0;
    {
        MutexGuard g(&m_mutex);
        auto it = m_transferset.find(tmptp);
        isfind = it != m_transferset.end();
        if (isfind)
        {
            targetuserid = it->targetuserid;
            if (targetuserid == reqmsg->objectID())
            {
                m_transferset.erase(it);
            }
        }
    }

    // 找到了, 并且找到的目标用户和reqmsg中的目标用户是一样的, 说明此验证
    // 消息是对的
    if (isfind && targetuserid == reqmsg->objectID())
    {
        // 检查群是否存在
        std::string cluacc;
        if (m_useroperator->getClusterAccount(clusterid, &cluacc) != 0)
        {
            //
            msg->appendContent(ANSC::CLUSTER_ID_ERROR);
            return r->size();
        }
        // else go through
        // 检查targetuser是否达到群数量的限制
        UserDao ud;
        if (!ud.isClusterLimitValid(targetuserid))
        {
            msg->appendContent(ANSC::CLUSTER_MAX_LIMIT);
            return r->size();
        }
        else
        {
            // ok
            uint8_t isSuccess = ANSC::FAILED;
            if (res == ANSC::ACCEPT)
            {
                if (m_useroperator->modifyClusterOwner(clusterid, //
                        reqmsg->objectID(), // new owner
                        userid) != 0) // old owner
                {
                    isSuccess = ANSC::FAILED;
                    msg->appendContent(ANSC::FAILED);
                }
                else
                {
                    isSuccess = ANSC::SUCCESS;
                    msg->appendContent(ANSC::SUCCESS);
                }
            }
            else
            {
                isSuccess = ANSC::SUCCESS;
                msg->appendContent(ANSC::SUCCESS);
            }
            genNotifyTransferCluster(userid, // old owner
                reqmsg->objectID(), // new owner
                clusterid, res, isSuccess, r);
        }
    }
    else
    {
        msg->appendContent(ANSC::FAILED);
    }

    return r->size();
}

void TransferDeviceCluster::genNotifyTageetUser(const TransferPair &tp, std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string useraccount;
    if (m_useroperator->getUserAccount(tp.userid, &useraccount) != 0)
        return;
    if (m_useroperator->getClusterAccount(tp.clusterid, &clusteraccount) != 0)
        return;

    std::vector<char> buf;
    ::WriteUint64(&buf, tp.clusterid);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, tp.userid);
    ::WriteString(&buf, useraccount);

    Notification notify;
    notify.Init(tp.targetuserid,
        ANSC::SUCCESS,
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_REQ,
        0,
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(tp.targetuserid);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
}

void TransferDeviceCluster::genNotifyTransferCluster(uint64_t oldowner,
        uint64_t newowner,
        uint64_t clusterid,
        uint8_t res,
        uint8_t issuccess,  // 数所库操作是否成功
        std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string newuseraccount;
    if (m_useroperator->getClusterAccount(clusterid, &clusteraccount) != 0)
        return;
    if (m_useroperator->getUserAccount(newowner, &newuseraccount) != 0)
        return;

    std::vector<char> buf;
    ::WriteUint8(&buf, res);
    ::WriteUint64(&buf, clusterid);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, newowner);
    ::WriteString(&buf, newuseraccount);
    Notification notify;
    notify.Init(oldowner,
        issuccess,
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_VERIRY,
        0,
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(oldowner);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}
