#include "modifyuserhead.h"
#include "messageencrypt.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "useroperator.h"

ModifyUserHead::ModifyUserHead(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int ModifyUserHead::processMesage(Message *reqmsg,
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
    if (len != 1)
    {
        ::writelog("modify head len error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    uint64_t uid = reqmsg->objectID();
    uint8_t header = ::ReadUint8(c);
    int res = m_useroperator->modifyUserHead(uid, header);
    uint8_t ans = res >= 0 ? ANSC::SUCCESS : ANSC::FAILED;
    msg->appendContent(ans);
    return r->size();
}
