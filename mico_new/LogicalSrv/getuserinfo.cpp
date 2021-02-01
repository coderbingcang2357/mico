#include "onlineinfo/icache.h"
#include "useroperator.h"
#include "getuserinfo.h"
#include "messageencrypt.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"

GetUserInfo::GetUserInfo(ICache *c, UserOperator *uop)
    : m_cache(c), m_userop(uop)
{
}

int GetUserInfo::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if (::decryptMsg(reqmsg, m_cache) < 0)
    {
        ::writelog("get user info decrypt failed.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    uint64_t userid;
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    if (plen != sizeof(userid))
    {
        ::writelog("get user info error msg.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    userid = ::ReadUint64(p);

    UserInfo *ui = m_userop->getUserInfo(userid);
    std::unique_ptr<UserInfo> del(ui); // for auto del
    if (ui == 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(ui->account);
    msg->appendContent(ui->phone);
    msg->appendContent(ui->mail);
    msg->appendContent(ui->headNumber);
    msg->appendContent(ui->signature);

    return r->size();
}
