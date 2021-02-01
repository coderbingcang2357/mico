#include <vector>
#include "util/util.h"
#include "Message/Notification.h"
#include "pushservice.h"
#include "protocoldef/protocol.h"
#include "Message/Message.h"

PushService::PushService(IMysqlConnPool *pool)
    : m_pushmessagedatabase(pool)
{
}

std::list<InternalMessage*> PushService::get(uint64_t userid)
{
    std::list<InternalMessage*> ret;
    std::list<PushMessage> messages = m_pushmessagedatabase.get(userid);
    for (auto &pushmessage : messages)
    {
        ret.push_back(pushMessaeToIntermessage(userid, pushmessage));

    }
    return ret;
}

void PushService::messageack(uint64_t userid, uint64_t msgid)
{
    m_pushmessagedatabase.messageAck(userid, msgid);
}

std::list<InternalMessage*> PushService::getPushMessage(const std::list<uint64_t> &receivers, uint64_t msgid)
{
    std::list<InternalMessage*> ret;
    PushMessage pushmessage = m_pushmessagedatabase.getPushMessage(msgid);
    if (!pushmessage)
        return ret;
    for (auto userid : receivers)
    {
        ret.push_back(pushMessaeToIntermessage(userid, pushmessage));
    }
    return ret;
}

InternalMessage *PushService::pushMessaeToIntermessage(uint64_t userid,
    const PushMessage &pushmessage)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, pushmessage.id); // push message id
    ::WriteString(&buf, pushmessage.message);
    Notification notify;
    notify.Init(userid, // receiver
        ANSC::SUCCESS,
        CMD::NTP_CMD_SERVER_PUSH_MESSAGE,
        0, //sn
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(userid);
    msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
    msg->appendContent(&notify);

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    return imsg;
}

