#include "feedback.h"
#include "useroperator.h"
#include "onlineinfo/icache.h"
#include "Message/Message.h"
//#include "Message/IntMsgHandler.h"
#include "messageencrypt.h"
#include "util/util.h"

SaveFeedBack::SaveFeedBack(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int SaveFeedBack::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if (::decryptMsg(reqmsg, m_cache) != 0)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    std::string text;
    int rl = ::readString(reqmsg->content(), reqmsg->contentLen(), &text);
    if (rl <= 0)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    if (m_useroperator->saveFeedBack(reqmsg->objectID(), text) != 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    msg->appendContent(ANSC::SUCCESS);
    return r->size();
}
