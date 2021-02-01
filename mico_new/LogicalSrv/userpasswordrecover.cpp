#include <memory>
#include <assert.h>
#include "thread/mutexguard.h"
#include "userpasswordrecover.h"
#include "useroperator.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"
#include "verfycodegen.h"
#include "onlineinfo/userdata.h"
#include "Crypt/MD5.h"
#include "Crypt/tea.h"
#include "mailsender.h"
#include "Message/Message.h"
#include "util/util.h"
#include "verfycodegen.h"
#include "dbaccess/user/userdao.h"
#include "server.h"
#include "timer/timer.h"

UserPasswordRecover::UserPasswordRecover(MicoServer *s, UserOperator *uop)
    : m_useroperator(uop), m_server(s), m_codeTimeout(30 * 60 * 1000)
{
}

int UserPasswordRecover::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    switch (reqmsg->commandID())
    {
    case CMD::CMD_PER__RECOVER_PWD1__REQ:
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(0);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        r->push_back(imsg);

        // mail, username
        ReqData rd;
        char *p = reqmsg->content();
        int plen = reqmsg->contentLen();
        int len = readString(p, plen, &rd.usermail);
        if (len <= 0)
        {
            msg->appendContent(ANSC::MESSAGE_ERROR);
            break;
            // error
        }
        p += len;
        len = readString(p, plen - len, &rd.useraccount);
        if (len <= 0)
        {
            msg->appendContent(ANSC::MESSAGE_ERROR);
            break;
            // error
        }
        p += len;

        ReqResult res = this->req(rd);
        msg->setObjectID(res.userid);
        msg->appendContent(&res);
    }
    break;

    case CMD::CMD_PER__RECOVER_PWD2__VERIFY:
    {
        VerfyData vd;
        VerfyResult vr;
        char *p = reqmsg->content();
        uint16_t plen = reqmsg->contentLen();
        int readlen = ::readString(p, plen, &vd.verfycodeFromClient);
        if (readlen > 1) // 1表示empty字符串, 验证码不可以为空
        {
            //ok 
            p += readlen;
            plen -= readlen;
            assert(plen == sizeof(vd.userid));
            if (plen == sizeof(vd.userid))
            {
                vd.userid = ::ReadUint64(p);
                vr = this->verify(vd);
            }
            else
            {
                // error
                ::writelog("reset pwd msgerr.1");
                vr.answccode = ANSC::MESSAGE_ERROR;
            }
        }
        else
        {
            // error
            ::writelog("reset pwd msgerr.2");
            vr.answccode = ANSC::MESSAGE_ERROR;
        }


        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setDestID(0);
        msg->appendContent(&vr);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        // OutputMsg(imsg, ToOutside);
        r->push_back(imsg);
    }
    break;

    case CMD::CMD_PER__RECOVER_PWD3__RESET:
    {
        SetPwdData spd;
        SetPwdResult sr;
        // spd.userid = reqmsg->ObjectID();
        char *p = reqmsg->content();
        uint16_t plen = reqmsg->contentLen();
        uint8_t keylen = uint8_t(*p);
        p++;
        plen--;
        if (plen != keylen + sizeof(spd.userid))
        {
            // error
            ::writelog("reset pwd msgerr.3");
            sr.answccode = ANSC::FAILED;
        }
        else
        {
            spd.keyfromclient = std::vector<char>(p, p + keylen);
            p += keylen;
            spd.userid = ::ReadUint64(p);
            sr = this->reset(spd);
        }

        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(sr.userid);
        msg->setDestID(0);
        msg->appendContent(&sr);

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        // OutputMsg(imsg, ToOutside);
        r->push_back(imsg);
    }
    break;
    }

    return r->size();
}

ReqResult UserPasswordRecover::req(const ReqData &d)
{
    UserInfo *ud = m_useroperator->getUserInfo(d.useraccount);
    std::unique_ptr<UserInfo> del__(ud); // for auto del
    ReqResult result;
    if (ud == 0)
    {
        result.answccode = ANSC::FAILED;
        result.userid = 0;
        ::writelog(InfoType, "Password Recover can not find user: %s",
            d.useraccount.c_str());

        return result;
    }

    if (ud->mail != d.usermail)
    {
        result.answccode = ANSC::EMAIL_NOT_EXIST;
        result.userid = 0;
        ::writelog(InfoType, "password recover mail error: %s, %s", d.useraccount.c_str(), d.usermail.c_str());
        return result;
    }

    //.
    std::string verfycode;
    ::genVerfyCode(4, &verfycode); // 4位验证码
    insertVerfycode(ud->userid, ud->account, verfycode);
    // 把验证码发到邮箱
    recoverPasswordSendVerfyCodeToMail(d.useraccount, d.usermail, verfycode);
    result.answccode = ANSC::SUCCESS;
    result.userid = ud->userid;
    result.verfycode= verfycode;
    return result;
}

VerfyResult UserPasswordRecover::verify(const VerfyData &d)
{
    VerfyResult result;
    std::string account;
    std::string verfycode;
    if (!this->getVerfyCode(d.userid, &account, &verfycode))
    {
        result.answccode = ANSC::VERIFY_CODE_EXPIRED;
        ::writelog(InfoType, "can not find verfycode: %lu", d.userid);
        return result;
    }
    if (verfycode != d.verfycodeFromClient)
    {
        // failed
        result.answccode = ANSC::VERIFY_CODE_ERROR;
        ::writelog(InfoType, "verfycode error: %lu", d.userid);
        return result;
    }
    // ok
    result.answccode = ANSC::SUCCESS;
    return result;
}

SetPwdResult UserPasswordRecover::reset(const SetPwdData &d)
{
    SetPwdResult result;
    std::string account;
    std::string verfycode;
    if (!this->getVerfyCode(d.userid, &account, &verfycode))
    {
        result.answccode = ANSC::VERIFY_CODE_EXPIRED;
        ::writelog(InfoType, "verfycode error: %lu", d.userid);
        result.userid = d.userid;
        return result;
    }

    // 验证码生成md5, 用来解密
    char key[16];
    MD5 md5;
    md5.input(verfycode.c_str(), verfycode.size());
    md5.output(key);

    //
    char bufpassword[1024];
    int len = tea_dec((char *)&d.keyfromclient[0], d.keyfromclient.size(),
        bufpassword,
        key,
        sizeof(key));
    assert(len == 16);
    if (len < 0)
    {
        ::writelog(InfoType, "set pwd tea_dec failed: %lu", d.userid);
        result.answccode = ANSC::FAILED;
        result.userid = d.userid;
        return result;
    }
    UserDao ud;
    int r = ud.modifyPassword(account,
                            d.userid,
                            byteArrayToHex((uint8_t *)bufpassword, len));
    // ok modify password in database
    //int r =
    //    m_useroperator->modifyPassword(d.userid, byteArrayToHex((uint8_t *)bufpassword, len));
    if (r != 0)
    {
        ::writelog(InfoType, "modify password failed: %lu", d.userid);
        result.userid = d.userid;
        result.answccode = ANSC::FAILED;
        return result;
    }

    removeVerfyCode(d.userid);

    // ok
    result.userid = d.userid;
    result.answccode = ANSC::SUCCESS;
    result.key = std::vector<char>(key, key + sizeof(key));
    return result;
}

void UserPasswordRecover::insertVerfycode(uint64_t userid,
    const std::string &account,
    const std::string &verfycode)
{
    RecoverVerifyCodeinfo rvi;
    rvi.account = account;
    rvi.verifycode = verfycode;
    Timer *t = new Timer(m_codeTimeout);
    t->setTimeoutCallback([userid, verfycode, this](){
            ::writelog("remove");
            this->removeVerfyCode(userid);
            ::writelog(InfoType, "reset pwd verfy code time out: %s, %lu",
                    verfycode.c_str(), userid);
    });
    rvi.timer = std::shared_ptr<Timer>(t);
    ::writelog(InfoType, "timer: %p", t);
    t->runOnce();

    MutexGuard g(&m_mutex);

    m_verfycode[userid] = rvi;
}

bool UserPasswordRecover::getVerfyCode(uint64_t userid,
    std::string *account,
    std::string *verfycode)
{
    MutexGuard g(&m_mutex);

    auto it = m_verfycode.find(userid);
    if (it != m_verfycode.end())
    {
        *account = it->second.account;
        *verfycode = it->second.verifycode;
        return true;
    }
    else
    {
        return false;
    }
}

void UserPasswordRecover::removeVerfyCode(uint64_t userid)
{
    MutexGuard g(&m_mutex);

    auto it = m_verfycode.find(userid);
    if (it != m_verfycode.end())
    {
        m_verfycode.erase(it);
    }
}

// TODO FIX ME这里最好用异步,因为发邮件可能很费时
void UserPasswordRecover::recoverPasswordSendVerfyCodeToMail(
    const std::string &acc,
    const std::string &mail,
    const std::string &verfycode)
{
    ::writelog("send mail.1");
    m_server->asnycMail([=](){
        MailSender mailsender;
        mailsender.setMailType(MailSender::EM_ResetPwd);
        mailsender.setTargetMailBox(mail);
        mailsender.setTargetAccount(acc);
        mailsender.setVerifCode(verfycode);
        mailsender.send();
    });
    return;
}

void ReqResult::toByteArray(std::vector<char> *out)
{
    // 只有一个应答码1字节, 用户id 8字节
    out->push_back(this->answccode);
    char *p = (char *)&userid;
    out->insert(out->end(), p, p + sizeof(userid));
}

void VerfyResult::toByteArray(std::vector<char> *out)
{
    // 只有一个应答码1字节
    out->push_back(this->answccode);
}


void SetPwdResult::toByteArray(std::vector<char> *out)
{
    out->push_back(this->answccode);
}
