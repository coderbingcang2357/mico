#ifndef USER_MODIFY_MAIL_H
#define USER_MODIFY_MAIL_H

#include <stdint.h>
#include <map>
#include <string>
#include <tuple>
#include "imessageprocessor.h"
#include "Message/itobytearray.h"
#include "Message/Message.h"
#include "thread/mutexwrap.h"
#include "onlineinfo/icache.h"
class UserOperator;
class MicoServer;
class Timer;

class ModifyMailReqData
{
public:
    uint64_t userid;
    std::string newmail;
};

class ModifyMailReqResult : public IToByteArray
{
public:
    void toByteArray(std::vector<char> *out);
    uint8_t answccode;
};

class ModifyMailVerfyData
{
public:
    uint64_t userid;
    std::string verfycode;
    std::string newmail;
};

class ModifyMailVerfyResult : public IToByteArray
{
public:
    void toByteArray(std::vector<char> *out);
    uint8_t answccode;
};

class ModifyMailVerifyCodeInfo
{
public:
    uint64_t userid;
    int times;
    std::string verifycode;
    std::string mail;
    std::shared_ptr<Timer> timer;
};

class UserModifyMail : public IMessageProcessor
{
public:
    UserModifyMail(MicoServer *s, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ModifyMailReqResult req(const ModifyMailReqData &d);
    ModifyMailVerfyResult verfy(const ModifyMailVerfyData &d);

    void removeVerifycode(uint64_t userid);
    void insertVerfyCode(uint64_t userid,
                const std::string &verfycode, const std::string &mail);
    bool getVerfyCode(uint64_t userid, // std::string *verfycode);
                ModifyMailVerifyCodeInfo *mmvi);

    void sendVerfycodeToMail(const std::string &mail,
        const std::string &verfycode);

    MutexWrap m_mutex;
    // std::map<uint64_t, std::pair<int, std::string> > m_verfycode;
    std::map<uint64_t, ModifyMailVerifyCodeInfo> m_verfycode;

    UserOperator *m_useroperator;
    MicoServer *m_server;
    int m_codetimeout;

    friend class TestUserModifyMail;
};
#endif
