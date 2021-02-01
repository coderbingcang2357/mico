#include "usermodifymail.h"
#include "verfycodegen.h"
#include "mailsender.h"
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "useroperator.h"
#include "messageencrypt.h"
#include "util/util.h"
#include "thread/mutexguard.h"
#include "server.h"
#include "timer/timer.h"

UserModifyMail::UserModifyMail(MicoServer *s, UserOperator *uop)
    : m_useroperator(uop), m_server(s), m_codetimeout(30 * 60 * 1000)
{
}

int UserModifyMail::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_server->onlineCache()) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setDestID(reqmsg->objectID());
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    switch (reqmsg->commandID())
    {
    case CMD::CMD_PER__MODIFY_MAILBOX1__REQ:
    {
        ModifyMailReqData rd;
        rd.userid = reqmsg->objectID();
        readString(reqmsg->content(), reqmsg->contentLen(), &rd.newmail);

        ModifyMailReqResult reqres = this->req(rd);

        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(reqmsg->objectID());
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(&reqres);
        // EncryptMsg(msg);
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);
    }
    break;

    case CMD::CMD_PER__MODIFY_MAILBOX2__VERIFY:
    {
        ModifyMailVerfyData vd;
        ModifyMailVerfyResult vr;

        vd.userid = reqmsg->objectID();
        char *p = reqmsg->content();
        int contentlen = reqmsg->contentLen();
        int len = readString(p, contentlen, &vd.verfycode);
        int maillen = readString(p + len, contentlen, &vd.newmail);
        if (len <= 0 || maillen <= 0)
        {
            // error 
            vr.answccode = ANSC::FAILED;
        }
        else
        {
            vr = this->verfy(vd);
        }



        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(reqmsg->objectID());
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(&vr);
        // EncryptMsg(msg);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(true);
        r->push_back(imsg);
    }
    break;
    }

    return r->size();
}

ModifyMailReqResult UserModifyMail::req(const ModifyMailReqData &d)
{
    ModifyMailReqResult result;
    std::string verfycode;
    ::genVerfyCode(4, &verfycode);
    UserInfo *u = m_useroperator->getUserInfo(d.userid);
    std::unique_ptr<UserInfo> del__(u); // for auto del

    insertVerfyCode(d.userid, verfycode, u->mail);

    sendVerfycodeToMail(u->mail, verfycode);
    result.answccode = ANSC::SUCCESS;

    return result;
}

ModifyMailVerfyResult UserModifyMail::verfy(const ModifyMailVerfyData &d)
{
    ModifyMailVerifyCodeInfo verfycode;
    ModifyMailVerfyResult result;
    if (!this->getVerfyCode(d.userid, &verfycode))
    {
        ::writelog(InfoType,
            "modify mail: can not find verfycode, %lu",
            d.userid);
        result.answccode = ANSC::VERIFY_CODE_EXPIRED;
        return result;
    }
    if (verfycode.times < 0)
    {
        removeVerifycode(d.userid);
        result.answccode = ANSC::VERIFY_CODE_EXPIRED;
        return result;
    }

    if (verfycode.verifycode != d.verfycode)
    {
        ::writelog(InfoType,
                "modify mail: error verfycode, %lu, %s",
                d.userid,
                verfycode.verifycode.c_str());
        result.answccode = ANSC::VERIFY_CODE_ERROR;
        return result;
    }


    if (verfycode.mail == d.newmail)
    {
        // 邮箱没有变
    }

    // modify in database
    int res;
    if ((res = m_useroperator->modifyMail(d.userid, d.newmail)) == 0)
    {
        result.answccode = ANSC::SUCCESS;

        removeVerifycode(d.userid);
    }
    else
    {
        result.answccode = res; // ANSC::Failed;
        ::writelog(InfoType,
            "modify mail, database operator failed. %lu",
            d.userid);
    }
    return result;
}

void UserModifyMail::removeVerifycode(uint64_t userid)
{
    MutexGuard g(&m_mutex);

    auto it = m_verfycode.find(userid);
    if (it != m_verfycode.end())
    {
        m_verfycode.erase(it);
    }
}

void UserModifyMail::insertVerfyCode(uint64_t userid,
                const std::string &verfycode, const std::string &mail)
    //const std::string &verfycode)
{
    ModifyMailVerifyCodeInfo  vi;
    vi.userid = userid;
    vi.verifycode = verfycode;
    vi.mail = mail;
    vi.times = 3;

    Timer *t = new Timer(m_codetimeout);
    t->setTimeoutCallback([userid, verfycode, this]{
            ::writelog("remove");
            this->removeVerifycode(userid);
            ::writelog(InfoType, "timeout remove verify code: %s, %lu",
                verfycode.c_str(), userid);
        });
    vi.timer = std::shared_ptr<Timer>(t);
    t->runOnce();
    ::writelog(InfoType, "timer:%p", t);

    MutexGuard g(&m_mutex);
    m_verfycode[userid] = vi;
}

bool UserModifyMail::getVerfyCode(uint64_t userid,
                ModifyMailVerifyCodeInfo *mmvi)
    // std::string *verfycode)
{
    MutexGuard g(&m_mutex);

    auto it = m_verfycode.find(userid);
    if (it != m_verfycode.end())
    {
        it->second.times--;
        *mmvi = it->second;
        // m_verfycode.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}

void UserModifyMail::sendVerfycodeToMail(const std::string &mail,
    const std::string &verfycode)
{
    m_server->asnycMail([=](){
        MailSender mailSender;
        mailSender.setMailType(MailSender::EM_ModifyMailBox);
        mailSender.setTargetMailBox(mail);
        mailSender.setTargetAccount("用户");
        mailSender.setVerifCode(verfycode);
        mailSender.send();
    });
}

void ModifyMailReqResult::toByteArray(std::vector<char> *out)
{
    out->push_back(answccode);
}

void ModifyMailVerfyResult::toByteArray(std::vector<char> *out)
{
    out->push_back(answccode);
}

