#include "messagefromnotifysrv.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "config/configreader.h"

SendNotify::SendNotify()
{

}

int SendNotify::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    Notification notify;
    if (!notify.fromByteArray(p, plen))
    {
        ::writelog("error message from notifisrv.!!");
        return 0;
    }
    if (notify.getNotifyType() == CMD::DNTP__TRANSFER_DEIVCES)
    {
        return deviceTransferNotify(&notify, r);
    }
    else if (notify.getNotifyType() == CMD::NTP_USER_LOGIN_AT_ANOTHER_CLIENT)
    {
        return userloginAtAnotherClientNotify(&notify, r);
    }

    Message *msg = Message::createClientMessage();
    msg->setCommandID(CMD::CMD_GEN__NOTIFY);
    msg->setObjectID(Configure::getConfig()->serverid);
    msg->setDestID(notify.GetObjectID());
    msg->appendContent(notify.answercode());
    msg->appendContent(notify.serialNumber());
    msg->appendContent(uint16_t(notify.getNotifyType()));
    msg->appendContent(notify.GetMsgBuf(), notify.GetMsgLen());

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(true);

    r->push_back(imsg);

#if 1
    ::writelog(InfoType, "Send notify to client:receiver: %lu, type: %u, sn: %lu",
                notify.GetObjectID(),
                notify.getNotifyType(),
                notify.serialNumber());
#endif

    return r->size();
}

int SendNotify::deviceTransferNotify(Notification *notify, std::list<InternalMessage *> *r)
{
    Message *msg = 0;
    IdType receivetype = GetIDType(notify->GetObjectID());
    if (receivetype  == IT_Device)
    {
        msg = Message::createDeviceMessage();
    }
    else if (receivetype == IT_User)
    {
        msg = Message::createClientMessage();
    }
    else
    {
        return 0;
    }
    msg->setCommandID(CMD::CMD_GEN__NOTIFY);
    msg->setObjectID(Configure::getConfig()->serverid);
    msg->setDestID(notify->GetObjectID());
    char *p = (char *)notify->GetMsgBuf();
    // 修改通知序列号
    p += notify->GetMsgLen();
    p -= 2; // 最后两字节是通知类型
    p -= 8; // 接着8字节是通知号(通知序列号)
    uint64_t *pton = (uint64_t *)p;
    *pton = notify->serialNumber();
    msg->appendContent(notify->GetMsgBuf(), notify->GetMsgLen());

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(true);

    r->push_back(imsg);
    return r->size();
}

int SendNotify::userloginAtAnotherClientNotify(Notification *notify,
    std::list<InternalMessage *> *r)
{
    Message *msg = Message::createClientMessage();
    char *msgbuf = (char *)notify->GetMsgBuf();
    char *key = msgbuf;
    uint32_t connserverid = 0;
    uint64_t connid = 0;
    sockaddrwrap addr;
    const char *ptoAddr = notify->GetMsgBuf() + 16 + 8; // 16这sessionkey长度, 8是userid长度
    const char *ptoconnserverid = notify->GetMsgBuf() + 16 + 8 + sizeof(addr);
    const char *ptoconnid = notify->GetMsgBuf() + 16 + 8 + sizeof(addr) + sizeof(connserverid);
    connserverid = ::ReadUint32(ptoconnserverid);
    connid = ::ReadUint64(ptoconnid);
    memcpy(&addr, ptoAddr, sizeof(addr));
    ::writelog(InfoType, "user login at another server notify: %s, connserverid: %u, connid: %lu",
               addr.toString().c_str(),
               connserverid,
               connid);
    msg->setSockerAddress(addr);
    msg->setCommandID(CMD::CMD_GEN__NOTIFY);
    msg->setObjectID(Configure::getConfig()->serverid);
    msg->setDestID(0);
    msg->setConnectionServerid(connserverid);
    msg->setConnectionId(connid);
    msg->appendContent(notify->answercode());
    msg->appendContent(notify->serialNumber());
    msg->appendContent(uint16_t(notify->getNotifyType()));
    msg->appendContent(notify->GetMsgBuf(), notify->GetMsgLen());
    msg->Encrypt(key, 16);

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(false);

    r->push_back(imsg);
    return r->size();
}

