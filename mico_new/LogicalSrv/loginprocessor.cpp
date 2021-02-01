#include "loginprocessor.h"
#include "util/logwriter.h"
#include "useroperator.h"
#include "sessionkeygen.h"
#include "Crypt/tea.h"
#include "Crypt/MD5.h"
#include "onlineinfo/userdata.h"
#include "config/configreader.h"
#include "onlineinfo/rediscache.h"
#include "onlineinfo/heartbeattimer.h"
#include "util/util.h"
#include "Message/Message.h"
#include "Message/Notification.h"
#include "gennotifyuseronlinestatus.h"
#include "sessionkeygen.h"
#include "deleteLocalCache.h"
#include "dbaccess/user/userdao.h"
#include "server.h"
#include "./loginatanotherclientnotify.h"
#include "pushmessage/pushservice.h"

LoginProcessor::LoginProcessor(ICache *c, MicoServer *s, UserOperator *uop, PushService *ps)
    : m_cacheManager(c), m_server(s), m_useroperator(uop), m_pushservice(ps)
{

}

int LoginProcessor::processMesage(Message *reqmesg, std::list<InternalMessage *> *r)
{
    std::vector<uint8_t> content;
    char buf[1024];
    Message *outmsg;
    switch (reqmesg->commandID())
    {
    case CMD::CMD_PER__LOGIN1__REQ:
    {
        std::string account;
        if (reqmesg->contentLen() != 0)
        {
            account = std::string(reqmesg->content());

#ifndef __TEST__
            ::writelog(InfoType, "account:%lu, %d\n", account.size(), reqmesg->contentLen());
#endif
        }
        else
        {
            ::writelog("login error msg");
        }
        LoginReqMessage loginreqmsg(reqmesg->objectID(), account);
        LoginReqResponseMessage resp = this->loginReq(&loginreqmsg);
        resp.toByteArray(&content);
        outmsg = new Message;
        outmsg->CopyHeader(reqmesg);
        outmsg->setObjectID(resp.userid);
        outmsg->setDestID(0);
        outmsg->appendContent(&(content[0]), content.size());

        InternalMessage *imsg = new InternalMessage(outmsg);
        imsg->setEncrypt(false);
        r->push_back(imsg);
        return r->size();
    }
    break;

    case CMD::CMD_PER__LOGIN2__VERIFY:
    {
        char *p = reqmesg->content();
        uint16_t len = reqmesg->contentLen();
        std::vector<uint8_t> data(p, p + len);
        LoginVerfyMessage vermsg;
        vermsg.d = data;
        vermsg.userid = reqmesg->objectID();
        vermsg.sockaddr = reqmesg->sockerAddress();
        vermsg.versionnumber = reqmesg->versionNumber();
        vermsg.connectionid = reqmesg->connectionId();
        vermsg.connectionServerid = reqmesg->connectionServerid();

        LoginResult loginres = this->loginVerfy(&vermsg, r);
        loginres.toByteArray(&content);
        
        // 加密
        len = tea_enc((char *)&content[0], content.size(),
                    buf,
                    (char *)&loginres.loginkey[0],
                    loginres.loginkey.size());
        // 
        outmsg = new Message;
        outmsg->CopyHeader(reqmesg);
        outmsg->setObjectID(reqmesg->objectID());
        outmsg->setDestID(0);
        outmsg->appendContent(buf, len);
        InternalMessage *imsg = new InternalMessage(outmsg);
        imsg->setEncrypt(false);
        r->push_back(imsg);
        if (loginres.answccode == ANSC::SUCCESS)
        {
            uint64_t userid = loginres.userdata->userid;
            MicoServer *server = this->m_server;
            // ..get notify from notify server
            getNotify(userid, r);
            m_server->asnycWork([userid, server, this](){
                std::list<InternalMessage *> resmsg;
                // post message user online
                GenNotifyUserOnlineStatus onlinestatus(userid, ONLINE, this->m_useroperator);
                onlinestatus.genNotify(&resmsg);
                this->getPushMessage(userid, &resmsg);
                server->sendMessage(resmsg);
            });
        }
        return r->size();
    }
    break;

    default:
        assert(false);
        return 0;
    }

    return r->size();
}

LoginReqResponseMessage LoginProcessor::loginReq(LoginReqMessage *reqmsg)
{
    uint64_t userID = reqmsg->userID();
    //UserOperator psOperator;
    LoginReqResponseMessage resp;
    resp.userid = userID;

    std::string useraccount = reqmsg->userAccount();

#ifndef __TEST__
    ::writelog(InfoType, "user req login: id: %lu, account: %s", userID, useraccount.c_str());
#endif
    // UserInfo *userinfo = 0;
    std::string pwd;
    UserDao ud;
    //用户通过ID请求
    if (userID != 0)
    {
        //userinfo = psOperator.getUserInfo(userID);
        //if (userinfo == 0)
        //{
        resp.answccode = ANSC::ACCOUNT_NOT_EXIST;
        return resp;
        //}
    }
    //用户通过帐号请求
    else if (useraccount.size() > 0)
    {
        //userinfo = psOperator.getUserInfo(useraccount);
        //if (userinfo == 0)
        //{
        //    resp.answccode = ANSC::AccountNotExist;
        //    return resp;
        //}
        if (ud.getPasswordAndID(useraccount, &userID, &pwd) == 0)
        {
            resp.userid = userID;
        }
        else
        {
            resp.answccode = ANSC::ACCOUNT_NOT_EXIST;
            return resp;
        }
    }
    else
    {
        resp.answccode = ANSC::MESSAGE_ERROR;
        return resp;
    }

    // assert(userinfo != 0);

    char loginKey[16] = {0};
    char md5Pwd[16] = {0};
    MD5 md5;
    // 数据库里面存的是16进制字符串, 所以要把16进制字符串转成字节
    // md5.stringTobytes(userinfo->pwd.c_str(), (byte*) md5Pwd);
    md5.stringTobytes(pwd.c_str(), (byte*) md5Pwd);

    this->genLoginKey(md5Pwd, LOGIN_KEY_FACTOR, loginKey);

    uint64_t randomNumber = getRandomNumber();

    LoginKeyInfo *info = new LoginKeyInfo(
        userID,// userinfo->userid,
        randomNumber,
        // std::shared_ptr<UserInfo>(userinfo),
        useraccount,
        std::vector<uint8_t>(loginKey, loginKey + 16),
        std::vector<uint8_t>(LOGIN_KEY_FACTOR, LOGIN_KEY_FACTOR + LOGIN_KEY_FACTOR_LEN));
    this->insertKey(info);

    resp.answccode = ANSC::SUCCESS;
    resp.randomnumber = randomNumber;
    resp.keyfactor = std::vector<uint8_t>(LOGIN_KEY_FACTOR, LOGIN_KEY_FACTOR + LOGIN_KEY_FACTOR_LEN);

    return resp;
}

LoginResult LoginProcessor::loginVerfy(LoginVerfyMessage *reqmsg, std::list<InternalMessage *> *retmsg)
{
    uint64_t userid = reqmsg->userid;
    //UserOperator useroperator;
    // std::shared_ptr<UserInfo> userdata;
    LoginResult loginresult;

    LoginKeyInfo *info = this->getKey(userid);
    if (info == 0)
    {
        ::writelog(InfoType, "Login getkey failed:%lu", userid);
        return loginresult;
    }
    // auto delete
    std::unique_ptr<LoginKeyInfo> _info(info);

    loginresult.loginkey = info->loginkey;

    // 用第一步中生成的loginKey解密客户端发来的随机数
    //
    char out[1024];
    int plainlen = tea_dec((char *)&((reqmsg->d)[0]),
                        reqmsg->d.size(),
                        out,
                        (char *)&((info->loginkey)[0]),
                        info->loginkey.size());
#ifdef __TEST__
    plainlen  = 1;
#endif
    if (plainlen >= 0)
    {
        uint64_t recvdRandomNum = ReadUint64(out);

#ifdef __TEST__
    info->randomnumber = recvdRandomNum = 1;
#endif
#ifndef __TEST__
        ::writelog(InfoType, "recvrandomnumber: %lu, %lu",
                recvdRandomNum, info->randomnumber);
#endif
        // 解密后, 两次随机数一样
        if(info->randomnumber == recvdRandomNum)
        {
            // userdata = info->userinfo;

            char sessionKey[16] = {0};  // 登录成功, 生成session
            ::genSessionKey(sessionKey, sizeof(sessionKey));

            UserData *loginuser= new UserData;
            loginuser->userid = userid;
            loginuser->setLoginTime(time(0)); // 登录时间
            loginuser->setLastHeartBeatTime(time(0));
            loginuser->account = info->useraccount;// userdata->account;
            loginuser->setSockAddr(reqmsg->sockaddr);
            loginuser->setVersionNumber(reqmsg->versionnumber);
            loginuser->lastSerialNumber = 0;
            loginuser->setConnectionId(reqmsg->connectionid);
            loginuser->setConnectionServerId(reqmsg->connectionServerid);

            //
            Configure *c = Configure::getConfig();
            loginuser->setLoginServer(c->serverid);

            loginresult.answccode = ANSC::SUCCESS;
            loginresult.userdata = std::shared_ptr<UserData>(loginuser);
            loginresult.userdata->setSessionKey(std::vector<char>(sessionKey,
                                sessionKey + sizeof(sessionKey)));
        }
        else
        {
            ::writelog(InfoType, "Login: %lu, error random number", userid);
            loginresult.answccode = ANSC::MESSAGE_ERROR;
        }
    }
    else
    {
        ::writelog(InfoType, "Login: %lu, decrypt failed.", userid);
        loginresult.answccode = ANSC::FAILED;
    }

    if (loginresult.answccode == ANSC::SUCCESS)
    {
        userOnline(&loginresult, retmsg);
    }
    return loginresult;
}

void LoginProcessor::userOnline(LoginResult *r, std::list<InternalMessage *> *retmsg)
{
    uint64_t userID = r->userdata->userid;
    shared_ptr<UserData> userData(r->userdata);
    assert(userData);
    assert(userData->sessionKey().size() == 16);

    // a user login mutiple time at same time, we should remove the previous
    // login info in that server
    processMutipleLogin(userID, retmsg);

    m_cacheManager->insertObject(userID, userData);

    // 注册一个超时定时器
    ::registerUserTimer(userData->userid);

    // 创建一个udp连接
    //m_server->createUdpConnection(userData->sockAddr(), userData->userid);

    // 在redis保存一条记录
    RedisCache::instance()->insertObject(userID, userData);

    // 更新最后登录时间
    time_t curt = time(0);
    m_server->asnycWork([curt, userID, this](){
            this->m_useroperator->updateUserLastLoginTime(userID, curt);
        });


#ifndef __TEST__
    char log[255];
    sprintf(log, "LogicalSrv: User online: %s.", userData->account.c_str());
    ::writelog(log);
#endif
}

void LoginProcessor::processMutipleLogin(uint64_t userid,
    std::list<InternalMessage *> *r)
{
    std::shared_ptr<IClient> c = RedisCache::instance()->getData(userid);
    if (!c)
        return;
    int loginserverid = c->loginServerID();
    Configure *cfg = Configure::getConfig();
    // 如果前一次登录也是在这台电脑上那么不用发DelCacheMessage
    if (cfg->serverid == loginserverid)
    {
        // 向前一次登录的地方发送通知: 在别处登录
        r->push_back(LoginAtAnotherClientNotifyGen::genNotify(c, userid));
        return;
    }
    else
    {
        // 发消息到另外的服务器, 让它删除locacache在线信息, 同时在那里生成一个用
        // 户在别处登录的通知消息
        r->push_back(DeleteLocalCache::getDelCacheMessage(userid, loginserverid));
    }
}

void LoginProcessor::getPushMessage(uint64_t userid,
    std::list<InternalMessage *> *r)
{
    r->splice(r->end(), m_pushservice->get(userid));
}

void LoginProcessor::genLoginKey(const char *md5Pwd, const char *factor, char *loginKey)
{
    for(int i = 0; i < 16; i++)
        loginKey[i] = md5Pwd[i] ^ factor[i];
}

void LoginProcessor::insertKey(LoginKeyInfo *info)
{
    MutexGuard g(&mutex);

    m_keyset[info->userid] = info;
}

LoginKeyInfo *LoginProcessor::getKey(uint64_t userid)
{
    MutexGuard g(&mutex);

    auto it = m_keyset.find(userid);
    if (it != m_keyset.end())
    {
        LoginKeyInfo *li = it->second;
        m_keyset.erase(it);
        return li;
    }
    else
    {
#ifdef __TEST__
        LoginKeyInfo *l = new LoginKeyInfo(
        userid,
        0,
        "",
        std::vector<uint8_t>(),
        std::vector<uint8_t>());
        int a;
        char *p = (char *)&a;
        l->loginkey = std::vector<uint8_t>(p, p + 1);
        l->keyfactor = std::vector<uint8_t>(p, p + 1);
        return l;
#endif
        return 0;
    }
}

void LoginProcessor::getNotify(uint64_t userID,  std::list<InternalMessage *> *r)
{
    Notification notify;
    notify.SetObjectID(userID);
    Message *getnotifymsg = new Message;
    getnotifymsg->setCommandID(CMD::CMD_GET_NOTIFY_WHEN_BACK_ONLINE);
    getnotifymsg->setObjectID(userID);
    getnotifymsg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(getnotifymsg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    //r->push_back(imsg);
    //getnotifymsg->appendContent(userID);
    r->push_back(imsg);
}
void LoginReqResponseMessage::toByteArray(std::vector<uint8_t> *out)
{
    // 1字节answcer code, 8字节random number, 16字节factor
    out->clear();
    out->push_back(uint8_t(this->answccode));
    char *p = (char *)&this->randomnumber;
    out->insert(out->end(), p, p + sizeof(this->randomnumber));
    if (keyfactor.size() > 0)
    {
        p = (char *)&this->keyfactor[0];
        out->insert(out->end(), p, p + this->keyfactor.size());
    }
}

void LoginResult::toByteArray(std::vector<uint8_t> *out)
{
    // 1字节应答码, 16字节sesssionkey
    out->clear();
    out->push_back(uint8_t(this->answccode));
    if (this->userdata)
    {
        std::vector<char> seskey = this->userdata->sessionKey();
        assert(seskey.size() == 16);
        char *p = &seskey[0];
        out->insert(out->end(), p, p + seskey.size());
    }
}

