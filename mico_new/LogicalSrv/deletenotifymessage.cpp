#include "deletenotifymessage.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "onlineinfo/icache.h"
#include "messageencrypt.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"
#include "pushmessage/pushservice.h"

DeleteNotify::DeleteNotify(ICache *c, PushService *ps)
    : m_cache(c), m_pushservice(ps)
{
}

// 当收到CMD::GEN_NOTIFY...这个消息时就发一个通知到NotifySrv中去删除一个通知
int DeleteNotify::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if (!this->decryptMessage(reqmsg))
    {
        return 0;
    }
    uint64_t notifysernum; // 序列号
    uint16_t notifyNumber; // 通知号
    uint64_t pushmessageid;
    if (reqmsg->contentLen() < sizeof(notifysernum) + sizeof(notifyNumber))
    {
        return 0;
    }
    char *p = reqmsg->content();
    uint16_t len = reqmsg->contentLen();

    notifysernum = ::ReadUint64(p);
    p += sizeof(notifysernum);
    len -= sizeof(notifysernum);

    notifyNumber = ::ReadUint16(p);
    p += sizeof(notifyNumber);
    len -= sizeof(notifyNumber);

    if (len == 8)
    {
        pushmessageid = ::ReadUint64(p);
        m_pushservice->messageack(reqmsg->objectID(), pushmessageid);
    }

    Notification notify;
    notify.Init(reqmsg->objectID(),
        ANSC::SUCCESS,
        notifyNumber, // 通知类型....
        notifysernum,// 要删除的通知序列号
        0,
        0);
    Message *msg = new Message;
    msg->setObjectID(reqmsg->objectID());
    msg->setCommandID(CMD::CMD_DELETE_NOTIFY); // 删除通知命令
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(false);
    imsg->setType(InternalMessage::ToNotify);

    r->push_back(imsg);

    return r->size();
}

bool DeleteNotify::decryptMessage(Message *msg)
{
    // key长度是16字节, 但是因为字符串要以0结尾, 所以要写17
    char key[17] = "I0IWwkCydXpwVyBi"; 
    if (msg->Decrypt(key) > 0) // 这个函数返回的是长度
    {
        return true;
    }

    else if (::decryptMsg(msg, m_cache) == 0) // 这个函数返回0表示成功
    {
        return true;
    }

    else
    {
        ::writelog(InfoType, "delete notify decrypt failed.2");
        return false;
    }
}
