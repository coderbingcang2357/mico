#ifndef USERPASSWORDRECOVER__H
#define USERPASSWORDRECOVER__H

#include <stdint.h>
#include <string>
#include <map>
#include <memory>
#include "thread/mutexwrap.h"
#include "imessageprocessor.h"
#include "Message/itobytearray.h"

class MicoServer;
class Timer;

class ReqData
{
public:
    std::string useraccount;
    std::string usermail;
};

class ReqResult : public IToByteArray
{
public:
    void toByteArray(std::vector<char> *out);

    uint8_t answccode;
    uint64_t userid;
    std::string verfycode;
};

class VerfyData
{
public:
    uint64_t userid;
    std::string verfycodeFromClient;
};

class VerfyResult : public IToByteArray
{
public:
    uint8_t answccode;
    void toByteArray(std::vector<char> *out);
};

class SetPwdData
{
public:
    uint64_t userid;
    std::vector<char> keyfromclient;
};

class SetPwdResult : public IToByteArray
{
public:
    uint8_t answccode;
    uint64_t userid;
    std::vector<char> key;
    void toByteArray(std::vector<char> *out);
};

class UserOperator;
class RecoverVerifyCodeinfo
{
public:
    std::string account;
    std::string verifycode;
    std::shared_ptr<Timer> timer;
};

class UserPasswordRecover : public IMessageProcessor
{
public:
    UserPasswordRecover(MicoServer *s, UserOperator *uop);

    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ReqResult req(const ReqData &d);
    VerfyResult verify(const VerfyData &d);
    SetPwdResult reset(const SetPwdData &d);

    void insertVerfycode(uint64_t userid,
                        const std::string &account,
                        const std::string &verfycode);
    bool getVerfyCode(uint64_t userid, std::string *account, std::string *verfycode);
    void removeVerfyCode(uint64_t userid);

    void recoverPasswordSendVerfyCodeToMail(const std::string &acc,
        const std::string &mail,
        const std::string &verfycode);

    MutexWrap m_mutex;
    std::map<uint64_t, RecoverVerifyCodeinfo> m_verfycode;

    UserOperator *m_useroperator;
    MicoServer *m_server;
    int m_codeTimeout;

    friend class TestUserPasswordRecover;
};
#endif
