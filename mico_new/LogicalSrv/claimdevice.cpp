#include "claimdevice.h"
#include "timer/timer.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "onlineinfo/icommand.h"
#include "Message/MsgWriter.h"
#include "Message/Message.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "onlineinfo/devicedata.h"
#include "onlineinfo/userdata.h"
#include "onlineinfo/onlineinfo.h"
#include "onlineinfo/rediscache.h"
//#include "../Notif/NotifRequest.h"
//#include "../Notif/NotifReqWriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include "claimmanager.h"
//#include "claimdviceTimeoutCommand.h"
#include <iostream>
#include <stdio.h>
#include "newmessagecommand.h"
#include "config/configreader.h"
#include "Message/Notification.h"
#include "messageencrypt.h"
#include "onlineinfo/isonline.h"

#ifndef TEST
static const int ResendTimeInterval = 3000;
#else
static const int ResendTimeInterval = 200;
#endif

class ClaimtimeoutProc : public Proc
{
public:
    ClaimtimeoutProc(uint64_t userid, uint64_t devid)
        : m_userid(userid), m_deviceid(devid)
    {}

    void run()
    {
        Message *msg = new Message;
        msg->appendContent(m_userid);
        msg->appendContent(m_deviceid);
        msg->setCommandID(CMD::INTERNAL_CLAIM_TIMER_RESEND);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::Internal);
        NewMessageCommand *cmd = new NewMessageCommand(imsg);
        MainQueue::postCommand(cmd);
    }

private:
    uint64_t m_userid;
    uint64_t m_deviceid;
};


ClaimDeviceProcessor::ClaimDeviceProcessor(ICache *c, UserOperator *uop)
    : m_cache(c),
    m_useroperator(uop),
    m_claimmanager(new ClaimManager)
{
}

int ClaimDeviceProcessor::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    if (reqmsg->commandID() == CMD::INTERNAL_CLAIM_TIMER_RESEND)
    {
        return resendCheckPassword(reqmsg, r);
    }
    else if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(reqmsg->destID()); // it should be this server id
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(ANSC::MESSAGE_ERROR);
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);
        ::writelog("Claim Decrypt Failed.\n");
        // assert(false);
        return r->size();
    }

    if (reqmsg->commandID() == CMD::CMD_DCL__CLAIM_DEVICE)
    {
        return req(reqmsg, r);
    }
    else if (reqmsg->commandID() == CMD::CMD_CLAIM_CHECK_PASSWORD)
    {
        return checkPasswordResult(reqmsg, r);
    }
    else
    {
        assert(false);
        return 0;
    }
}

int ClaimDeviceProcessor::req(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    uint64_t claimUser = reqmsg->objectID();
    uint64_t deviceID;
    std::string password;
    if (plen <= sizeof(deviceID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    deviceID = ReadUint64(p);
    p += sizeof(deviceID);
    plen -= sizeof(deviceID);

    int readlen = ::readString(p, plen, &password);
    if (readlen <= 0)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    //
    ClaimDevice cd(claimUser, deviceID, password);
    if (m_claimmanager->isExist(&cd))
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    IsOnline isonline(m_cache, RedisCache::instance());
    bool useronline = isonline.isOnline(claimUser);
    bool devonline = isonline.isOnline(deviceID);
    if (!useronline|| !devonline)
    {
        msg->appendContent(ANSC::DEVICE_OFFLINE);
        return r->size();
    }

    // 是否要检查用户和设备加入了同一个群???

    // ok
    msg->appendContent(ANSC::SUCCESS);

    std::shared_ptr<ClaimDevice> newcd(new ClaimDevice(claimUser, deviceID, password));
    m_claimmanager->insertClaimRequest(newcd);
    // register Timer
    Timer *t = new Timer(ResendTimeInterval); // 3 秒
    t->setTimeoutCallback(new ClaimtimeoutProc(claimUser, deviceID));
    newcd->setTimer(t);
    t->start();

    sendCheckPasswordToDevice(newcd, r);

    return r->size();
}

int ClaimDeviceProcessor::checkPasswordResult(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    uint8_t prefix[2]; // 0xff, 0x03
    uint64_t userid;
    uint64_t deviceid;
    uint8_t step4; // 0x04
    uint8_t result;
    uint32_t len = sizeof(prefix)
                    + sizeof(userid)
                    + sizeof(deviceid)
                    + sizeof(step4)
                    + sizeof(result);
    if (plen != len)
    {
        ::writelog("claim: error device message checkpwd.");
        // failed
        return 0;
    }
    p += sizeof(prefix);

    userid = ReadUint64(p);
    p += sizeof(userid);

    deviceid = ReadUint64(p);
    p += sizeof(deviceid);

    step4 = *p;
    p++;

    result = *p;

    if (result == ANSC::SUCCESS)
    {
        if (m_useroperator->claimDevice(userid, deviceid) != 0)
            result = ANSC::FAILED;
        genNotifyUserClaimResult(userid, deviceid, result, r);
    }
    else
    {
        // failed
        genNotifyUserClaimResult(userid, deviceid, ANSC::DEVICE_PASSWORD_ERROR, r);
    }

    m_claimmanager->removeClaimRequest(userid, deviceid);

    return r->size();
}

int ClaimDeviceProcessor::sendCheckPasswordToDevice(const std::shared_ptr<ClaimDevice> &cd,
    std::list<InternalMessage *> *r)
{
    cd->incResendTimes();
    Message *msg = Message::createDeviceMessage();
    uint64_t serverid = Configure::getConfig()->serverid;
    msg->setDestID(cd->deviceID());
    msg->setObjectID(serverid);
    msg->setCommandID(CMD::CMD_CLAIM_CHECK_PASSWORD);
    msg->appendContent(uint8_t(0xff));
    msg->appendContent(uint8_t(0x03));
    msg->appendContent(cd->userID());
    msg->appendContent(cd->deviceID());
    msg->appendContent(uint8_t(0x03));
    msg->appendContent(cd->password());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    return r->size();
}

int ClaimDeviceProcessor::resendCheckPassword(Message *msg,
    std::list<InternalMessage *> *r)
{
    uint64_t userid;
    uint64_t devid;
    if (msg->contentLen() != sizeof(userid) + sizeof(devid))
        return 0;
    char *p = msg->content();
    userid = ReadUint64(p);
    p += sizeof(userid);
    devid = ReadUint64(p);
    p += sizeof(devid);
    std::shared_ptr<ClaimDevice> cd = m_claimmanager->get(userid, devid);
    if (!cd)
        return 0;
    if (cd->resendTimes() >= 3)
    {
        //
        genNotifyUserClaimResult(userid, devid, ANSC::DEVICE_NO_RESP, r);
        m_claimmanager->removeClaimRequest(userid, devid);
        return r->size();
    }
    else
    {
        return sendCheckPasswordToDevice(cd, r);
    }
}

void ClaimDeviceProcessor::genNotifyUserClaimResult(uint64_t userid,
    uint64_t devid,
    uint8_t res,
    std::list<InternalMessage *> *r)
{
    std::string devname;
    m_useroperator->getDeviceName(devid, &devname);
    std::vector<char> buf;
    ::WriteUint64(&buf, devid);
    ::WriteUint64(&buf, res);
    ::WriteString(&buf, devname);
    Notification notify;
    notify.Init(userid,
        ANSC::SUCCESS,
        CMD::CCMD__DEVICE_CLAIM,
        0,  // 在别的地方生成
        buf.size(),
        &buf[0]);
    Message *msg = new Message;
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->setObjectID(userid);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
    r->size();
}

ClaimDevice::~ClaimDevice()
{
    delete m_timer;
}
