#ifndef LOGINPROCESSOR__H
#define LOGINPROCESSOR__H

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <arpa/inet.h>
#include "protocoldef/protocol.h"
#include "onlineinfo/icache.h"
#include "imessageprocessor.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"

const char* const LOGIN_KEY_FACTOR     = "0123456789ABCDEF";
const uint16_t    LOGIN_KEY_FACTOR_LEN = 16;

class UserInfo;
class MicoServer;
class UserOperator;
class PushService;

class LoginReqMessage
{
public:
    LoginReqMessage(uint64_t userid, std::string account)
        : m_userid(userid), m_userAccount(account) {}

    uint64_t userID() { return m_userid; }
    std::string userAccount() { return m_userAccount; }

private:
    uint64_t m_userid;
    std::string m_userAccount;
};

class LoginReqResponseMessage
{
public:
    void toByteArray(std::vector<uint8_t> *out);

    uint8_t answccode;
    uint64_t userid;
    uint64_t randomnumber;
    std::vector<uint8_t> keyfactor;
};

class LoginVerfyMessage
{
public:
    uint64_t userid;
    std::vector<uint8_t> d;
    sockaddrwrap sockaddr;
    uint32_t versionnumber;
    int connectionServerid = 0;
    uint64_t connectionid = 0;
};

class LoginResult
{
public:
    LoginResult() : answccode(ANSC::FAILED), userdata(0) {}

    void toByteArray(std::vector<uint8_t> *out);

    uint8_t answccode;
    std::vector<uint8_t> loginkey;
    std::shared_ptr<UserData> userdata;
};

class LoginKeyInfo
{
public:
    LoginKeyInfo(uint64_t _userid,
                uint64_t _randomnumber,
                //std::shared_ptr<UserInfo> ui,
                const std::string &acc,
                const std::vector<uint8_t> &_loginkey,
                const std::vector<uint8_t> &_keyfactor)
            :   userid(_userid),
                randomnumber(_randomnumber),
                //userinfo(ui),
                useraccount(acc),
                loginkey(_loginkey),
                keyfactor(_keyfactor)
    {
    }
    uint64_t userid;
    uint64_t randomnumber;
    std::string useraccount;
    //std::shared_ptr<UserInfo> userinfo;
    std::vector<uint8_t> loginkey;
    std::vector<uint8_t> keyfactor;
};

class LoginProcessor : public IMessageProcessor
{
public:
    LoginProcessor(ICache *c, MicoServer *s, UserOperator *uop,
        PushService *ps);

    int processMesage(Message *, std::list<InternalMessage *> *r);


private:
    LoginReqResponseMessage loginReq(LoginReqMessage*);
    LoginResult loginVerfy(LoginVerfyMessage *, std::list<InternalMessage *> *retmsg);

    void userOnline(LoginResult *r, std::list<InternalMessage *> *retmsg);

    void insertKey(LoginKeyInfo *);
    LoginKeyInfo *getKey(uint64_t userid);
    void genLoginKey(const char *md5Pwd, const char *factor, char *loginKey);

    void getNotify(uint64_t userID,  std::list<InternalMessage *> *r);
    void genNotifyUserOnline(uint64_t userID,  std::list<InternalMessage *> *r);
    void processMutipleLogin(uint64_t userid, std::list<InternalMessage *> *r);
    void getPushMessage(uint64_t userid, std::list<InternalMessage *> *r);

    MutexWrap mutex;
    std::map<uint64_t, LoginKeyInfo *> m_keyset;
    ICache *m_cacheManager;
    MicoServer *m_server;
    UserOperator *m_useroperator;
    PushService *m_pushservice;
};

#endif

