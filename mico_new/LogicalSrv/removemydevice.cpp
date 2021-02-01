#include "removemydevice.h"
#include "useroperator.h"
#include "Message/Message.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "messageencrypt.h"
#include "sceneConnectDevicesOperator.h"

RemoveMyDevice::RemoveMyDevice(ICache *ic, UserOperator *op)
    : m_cache(ic), m_op(op)
{
}

int RemoveMyDevice::processMesage(Message *reqmsg,
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
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    char *c = reqmsg->content();
    uint32_t len = reqmsg->contentLen();

    if (len < 2)
    {
        // message error
        ::writelog(InfoType, "remove my device error len");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    uint16_t size = ::ReadUint16(c);
    len -= sizeof(size);
    c += sizeof(size);

    uint64_t deviceid = 0;
    if (len != size * sizeof(deviceid))
    {
        ::writelog(InfoType, "remove my device error size");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::list<uint64_t> deviceids;
    for (int i = 0; i < size; ++i)
    {
        deviceid = ::ReadUint64(c + i * sizeof(deviceid));
        deviceids.push_back(deviceid);
    }
    uint64_t userid = reqmsg->objectID();
    if (m_op->removeMyDevice(userid, deviceids) != 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    msg->appendContent(ANSC::SUCCESS);

    // 设备取消授权后,那么原来场景里面连接的设备,可能会有一些变得不可用
    // 所以要把这些不可用的设备从场景的连接里面删除掉
    SceneConnectDevicesOperator sceneop(m_op);
    sceneop.findInvalidDevice(userid);

    return r->size();
}
