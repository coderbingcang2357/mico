#include <assert.h>
#include <string>
#include <vector>
#include <algorithm>
#include "onlineinfo/isonline.h"
#include "usermessageprocess.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"
#include "protocoldef/protocolversion.h"
#include "useroperator.h"
#include "onlineinfo/onlineinfo.h"
#include "messageencrypt.h"
#include "useroperator.h"
#include "util/util.h"
#include "onlineinfo/userdata.h"
#include "onlineinfo/rediscache.h"
#include "Crypt/tea.h"
#include "onlineinfo/heartbeattimer.h"
#include "Message/Notification.h"
#include "gennotifyuseronlinestatus.h"
#include "sceneConnectDevicesOperator.h"
#include "transferdevicetoanothercluster.h"
#include "dbaccess/user/userdao.h"
#include "dbaccess/cluster/clusterdal.h"
#include "server.h"

UserRegister::UserRegister(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int UserRegister::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    InternalMessage *intmsg = new InternalMessage(msg);
    intmsg->setEncrypt(false);
    r->push_back(intmsg);

    // 
    uint8_t answccode;
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    if (plen <= 16) //密码一定是16字节加上其它的一些字段, 所以一定是大于16的
    {
        answccode = ANSC::MESSAGE_ERROR;
        msg->appendContent(&answccode, sizeof(uint8_t));
        return r->size();
    }

    // 16字节密码
    std::vector<char> pwd;
    pwd.insert(pwd.end(), p, p + 16);
    p += 16;

    // 用密码对剩下的数据解密
    int remainlen = plen - 16;
    char buf[1024];
    char *ptobuf = buf;
    int declen = tea_dec(p, remainlen, buf, (char *)&pwd[0], pwd.size());

    if (declen <= 0)
    {
        ::writelog("register user dec failed");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    // read username
    std::string username;
    std::string mail;
    int len;
    // mail
    len = readString(ptobuf, declen, &mail);
    ptobuf += len;

    if (len <= 1)
    {
        ::writelog("register user read mail failed.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    // username
    len = readString(ptobuf, declen - len, &username);
    ptobuf += len;

    if (len <= 1)
    {
        ::writelog("register user read username failed");
        return r->size();
    }

    assert(declen + buf == ptobuf);
    uint64_t userid;
    if (m_useroperator->registerUser(username,
                            mail,
                            //std::string(&pwd[0],pwd.size()),
                            byteArrayToHex((uint8_t *)&pwd[0], pwd.size()),
                            &userid) == 0)
    {
        // ok
        answccode = ANSC::SUCCESS;
    }
    else
    {
        // failed
        answccode = ANSC::FAILED;
    }

    msg->appendContent(answccode);
    return r->size();
}

// user logout
UserLogoutProcessor::UserLogoutProcessor(MicoServer *c, UserOperator *uop)
    : m_server(c), m_useroperator(uop)
{

}

int UserLogoutProcessor::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_server->onlineCache()) != 0)
    {
        ::writelog("loginout decrypt failed.");
        // 解密失败
        return 0;
    }

    if (reqmsg->contentLen() < sizeof(uint64_t))
    {
        ::writelog("loginout content error.");
        return 0;
    }
    uint64_t usertologout = ReadUint64(reqmsg->content());

    ::unregisterTimer(usertologout);
    // remove udp connection
    auto userdata = m_server->onlineCache()->getData(usertologout);
    if (userdata)
    {
        //m_server->removeUdpConnection(userdata->sockAddr(), usertologout);

        time_t logintime = userdata->loginTime();
        // update online time
        time_t onlinetime = time(0) - logintime;
        m_server->asnycWork([onlinetime, usertologout, this](){
            this->m_useroperator->updateUserOnlineTime(usertologout, onlinetime);
            });
    }

    // remove from local cache
    m_server->onlineCache()->removeObject(usertologout);
    // remove from redis
    RedisCache::instance()->removeObject(usertologout);


    GenNotifyUserOnlineStatus gen(usertologout, OFFLINE, m_useroperator);
    gen.genNotify(r);
    return r->size();
}

UserOffline::UserOffline(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int UserOffline::processMesage(Message *msg, std::list<InternalMessage *> *r)
{
    uint64_t useroffline = msg->objectID();
    ::writelog(InfoType, "user offline gennotify: %lu", useroffline);
    GenNotifyUserOnlineStatus gen(useroffline, OFFLINE, m_useroperator);
    gen.genNotify(r);
    return r->size();
}

// user modify password
UserModifyPassword::UserModifyPassword(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int UserModifyPassword::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    // new password
    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    // 因为密码一定是16字节的
    if (plen != 16)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    shared_ptr<IClient> c = m_cache->getData(reqmsg->objectID());
    UserDao ud;
    int isok = ud.modifyPassword(c->name(),
                reqmsg->objectID(),
                byteArrayToHex((uint8_t *)p, plen)); // new password
    uint8_t ans;
    if (isok != 0)
    {
        ans = ANSC::FAILED;
    }
    else
    {
        ans = ANSC::SUCCESS;
    }
    //m_useroperator->modifyPassword(reqmsg->ObjectID(), byteArrayToHex((uint8_t *)p, plen));
    msg->appendContent(ans);
    return r->size();
}

// user modify nickname
ModifyNickName::ModifyNickName(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int ModifyNickName::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
// InternalMessage *ModifyNickName::processMesage(Message *reqmsg)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    // 1字节结果
    r->push_back(new InternalMessage(msg));
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    assert(reqmsg->commandID() == CMD::CMD_PER__MODIFY_NICKNAME);
    std::string newnickname;//(reqmsg->Content(), reqmsg->ContentLen());
    int rl = readString(reqmsg->content(), reqmsg->contentLen(), &newnickname);
    if (rl <= 0) // empty string
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        ::writelog(InfoType, "err msg: command %x", reqmsg->commandID());
        return r->size();
    }

    // TODO FIXME newnickname是否合法检查
    uint64_t userid = reqmsg->objectID();
    uint8_t result;
    if (m_useroperator->modifyNickName(userid, newnickname) == -1)
    {
        result = ANSC::FAILED;
    }
    else
    {
        result = ANSC::SUCCESS;
    }

    msg->appendContent(&result, sizeof(result));
    return r->size();
}


//--------------修改签名--------------------------------------------------
ModifySignature::ModifySignature(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int ModifySignature::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    // 1字节结果
    r->push_back(new InternalMessage(msg));
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("decrypt failed.");
        msg->appendContent(ANSC::FAILED);
        // 解密失败
        return 0;
    }
    assert(reqmsg->commandID() == CMD::CMD_PER__MODIFY_SIGNATURE);
    std::string newsignature(reqmsg->content(), reqmsg->contentLen());
    uint64_t userid = reqmsg->objectID();
    uint8_t result;
    if (m_useroperator->modifySignature(userid, newsignature) != -1)
    {
        result = ANSC::SUCCESS;
    }
    else
    {
        result = ANSC::FAILED;
    }

    msg->appendContent(&result, sizeof(result));
    return r->size();
}

//----------------------------------------------------------------------------
GetMyDeviceList::GetMyDeviceList(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

//InternalMessage *GetMyDeviceList::processMesage(Message *reqmsg)
int GetMyDeviceList::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("get my device list decrypt msg failed.");
        // 解密失败
        return 0;
    }
    if (reqmsg->versionNumber() >= 0x180)
    {
        r->push_back(new InternalMessage(getMyDeviceList_V180(reqmsg)));
        return r->size();
    }
    else
    {
        return 0;
    }
}

Message *GetMyDeviceList::getMyDeviceList_V180(Message *reqmsg)
{
    std::vector<DeviceInfo> vDeviceInfo;

    char *d = reqmsg->content();
    uint8_t answerCode = ANSC::SUCCESS;
    // 2是因为列表起始位置占2字节
    if (reqmsg->contentLen() < 2)
    {
        ::writelog("get mydevicelist v2 : request messqge length error");
        answerCode = ANSC::FAILED;
    }

    uint16_t startpos = 0;
    if (answerCode != ANSC::FAILED)
    {
        // if (reqmsg->VersionNumber() == 2) 
        startpos = ReadUint16(d);

        if(this->m_useroperator->getMyDeviceList(
                reqmsg->objectID(), &vDeviceInfo) < 0)
        {
            answerCode = ANSC::FAILED;
            vDeviceInfo.clear();
        }
    }

    uint16_t alldevicecount = vDeviceInfo.size();

    if (startpos > vDeviceInfo.size())
        startpos = vDeviceInfo.size();

    uint16_t deviceCount = 0;

    uint16_t protoversion = ::getProtoVersion(reqmsg->versionNumber());
    // .
    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->objectID());
    responseMsg->appendContent(answerCode);
    // 设备总数2字节
    responseMsg->appendContent(alldevicecount);
    // 本次响应的设备数, 这里先写个0, 后面会修改它
    responseMsg->appendContent(deviceCount);

    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (protoversion >= 0x202)
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }

    // 跳过起始位置之前的
    uint32_t i = startpos;
    IsOnline isonline(m_cache, RedisCache::instance());
    for(; i < vDeviceInfo.size(); ++i)
    {
        uint32_t tmp = responseMsg->contentLen()
            + Message::SIZE_HEADER_AND_ENDING;

        DeviceInfo &di = vDeviceInfo[i];
        tmp += sizeof(di.id);
        tmp = tmp + sizeof(di.clusterID);
        tmp = tmp + sizeof(di.macAddr);;
        //  device type, proto version 201 添加的字段
        if (protoversion == uint16_t(0x201)
            || protoversion == uint16_t(0x202))
        {
            tmp = tmp + sizeof(uint8_t); // type
        }
        tmp = tmp + di.name.size() + 1;

        if (tmp > maxmsgsize)
        {
            break;
        }

        ++deviceCount;
        responseMsg->appendContent(di.id);
        responseMsg->appendContent(di.clusterID);
        //char macchar[16];
        //uint64ToMacAddr(di.macAddr, macchar);
        responseMsg->appendContent(&di.macAddr[0], 6);
        //  device type, proto version 201 添加的字段
        if (protoversion == uint16_t(0x201)
                || protoversion == uint16_t(0x202))
        {
            responseMsg->appendContent(uint8_t(di.type));
        }
        // 在线状态
        uint8_t onlineStatus = isonline.isOnline(di.id) ? ONLINE : OFFLINE;
        responseMsg->appendContent(onlineStatus);
        responseMsg->appendContent(di.name);
    }

    // 修改设备应答数的值
    if (deviceCount != 0)
    {
        uint16_t *p = (uint16_t *)(responseMsg->content() + 1 + 2);
        *p = deviceCount;
    }

    // 加密
    // ::encryptMessage(responseMsg, m_cache);
    return responseMsg;
}

//---------------------修改设备notename
ModifyDeviceNoteName::ModifyDeviceNoteName(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int ModifyDeviceNoteName::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        return 0;
    }
    char* pos = reqmsg->content();

    uint64_t devClusterID = ReadUint64(pos);
    uint64_t deviceID = ReadUint64(pos += sizeof(devClusterID));
    string noteName(pos += sizeof(deviceID));
    uint8_t result = ANSC::FAILED;

    bool isAdmin = false;
    if(m_useroperator->checkClusterAdmin(reqmsg->objectID(),
            devClusterID,
            &isAdmin) < 0)
    {
        result = ANSC::FAILED;
    }
    else if(!isAdmin)
    {
        result = ANSC::AUTH_ERROR;
    }
    // else ok

    if(m_useroperator->modifyNoteName(deviceID, noteName) < 0)
    {
        result = ANSC::FAILED;
    }
    else
    {
        result = ANSC::SUCCESS;
    }

    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->destID());
    responseMsg->appendContent(&result, Message::SIZE_ANSWER_CODE);
    responseMsg->appendContent(&deviceID, sizeof(deviceID));

    r->push_back(new InternalMessage(responseMsg));
    return r->size();
}

// 用户心跳
UserHeartBeat::UserHeartBeat(MicoServer *ms, ICache *c, UserOperator *uop)
    : m_ms(ms), m_cache(c), m_useroperator(uop)
{
}

int UserHeartBeat::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("user heart beat decrypt failed");
        // 解密失败
        return 0;
    }
    uint64_t userID = reqmsg->objectID();
    if ((reqmsg->contentLen() != sizeof(userID) + sizeof(uint16_t))
        && (reqmsg->contentLen() != sizeof(userID)))
    {
        ::writelog("user heart beat data error");
        return 0;
    }
    char *ptocontent = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    uint64_t userid2 = ReadUint64(ptocontent);
    if (userID != userid2)
    {
        ::writelog("user heart beat invalue value");
        return 0;
    }
    ptocontent += sizeof(uint64_t);
    plen -= sizeof(uint64_t);

    uint16_t clientversion = 0;
    if (plen == 2)
    {
        clientversion = ::ReadUint16(ptocontent);
        //::writelog(InfoType, "read client version: %u", clientversion);
    }

    //cout << "Logical Server UserHeartBeat, ID: " << userID << endl;

    //CacheManager cacheManager;
    //UserData userData = cacheManager.GetUserData(userID);
    shared_ptr<UserData> userData
        = static_pointer_cast<UserData>(m_cache->getData(userID));//cacheManager.GetUserData(userID);
    //if(userData->Empty())
    if(!userData)
    {
        ::writelog(InfoType,
                "user heart beat: user can not find in cache: %lu\n",
                userID);
        return 0;
    }

    userData->setLastHeartBeatTime(time(NULL));
    userData->setVersionNumber(reqmsg->versionNumber());
    userData->setSockAddr(reqmsg->sockerAddress());
    uint16_t oldclientver = userData->clientVersion();

    if (clientversion != 0)
    {
        userData->setClientVersion(clientversion);
        if (oldclientver == 0)
        {
            m_ms->asnycWork([userData](){
                ::RedisCache::instance()->insertObject(userData->userid,
                            userData);
                });
        }
    }

    // 更新cache
    m_cache->insertObject(userID, userData);

    Message* responseMsg = new Message;
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());
    responseMsg->appendContent(userID);
    // responseMsg->SetContentLen(0);
    // EncryptMsg(responseMsg);
    InternalMessage *imsg = new InternalMessage(responseMsg);
    r->push_back(imsg);
    return r->size();
}

GetClientIPAndPort::GetClientIPAndPort(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetClientIPAndPort::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{

    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("transfer message decrypt failed, error meeeage.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    uint64_t userid = reqmsg->objectID();
    shared_ptr<UserData> userData
        = static_pointer_cast<UserData>(
                m_cache->getData(userid));
    //if(userData.Empty())
    if(!userData)
    {
        ::writelog("get client ip port user not online.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    //sockaddrwrap extranetSockAddr = userData->sockAddr();
    ::writelog(ErrorType, "get client ip and port not suppert.");
    // !!IMP
    uint32_t clientExtranetIP = 0;//extranetSockAddr.sin_addr.s_addr;
    uint16_t clientExtranetPort = 0;//extranetSockAddr.sin_port;

    msg->appendContent(ANSC::FAILED);
    msg->appendContent(clientExtranetIP);
    msg->appendContent(clientExtranetPort);

    return r->size();
}

//-------创建设备群
CreateDeviceCluster::CreateDeviceCluster(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int CreateDeviceCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message* responseMsg = NULL;
    responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());
    r->push_back(new InternalMessage(responseMsg));
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("create cluster decrypt failed.");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    //std::string clusterAccount(reqmsg->Content());
    //std::string description(reqmsg->Content() + clusterAccount.length() + 1);
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    std::string clusterAccount;
    std::string description;

    int readlen = ::readString(p, plen, &clusterAccount);
    p += readlen;
    plen -= readlen;
    int readdesclen = ::readString(p, plen, &description);
    p += readdesclen;
    plen -= readdesclen;

    // 如果readlen是1说明读出的是一个空字符串, 这肯定是不允许的
    // 描述可以为空, 0表示出错
    if (readlen <= 1 || readdesclen <= 0)
    {
        ::writelog("create cluster: read string failed");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    uint64_t userid = reqmsg->objectID();
    UserDao ud;
    // 检查是否超过限制, 最多创建(拥用)100个群
    if  (!ud.isClusterLimitValid(userid))
    {
        responseMsg->appendContent(ANSC::CLUSTER_MAX_LIMIT);
        return r->size();
    }

    // 检查群号是否存在
    bool isExisted = false;
    uint8_t result;
    uint64_t newClusterID = 0;

    if(m_useroperator->checkClusterAccount(clusterAccount, &isExisted) < 0)
    {
        // data base error
        result = ANSC::FAILED;
    }
    else
    {
        if (isExisted)
        {
            result = ANSC::ACCOUNT_USED;
        }
        else
        {
            if(m_useroperator->insertCluster(reqmsg->objectID(),
                    clusterAccount,
                    description,
                    &newClusterID) < 0)
            {
                // data base error
                result = ANSC::FAILED;
            }
            else
            {
                // ok
                result = ANSC::SUCCESS;
            }
        }
    }
    ::writelog(InfoType, "create cluster: %u, %lu", result, newClusterID);
    responseMsg->appendContent(result);
    responseMsg->appendContent(newClusterID);

    return r->size();
}

//--删除设备群
DeleteDeviceCluster::DeleteDeviceCluster(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int DeleteDeviceCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("del cluster decrypt failed");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t userid = reqmsg->objectID();
    uint64_t deviceclusterid;
    if (plen < sizeof(deviceclusterid))
    {
        ::writelog("delete dev cluster message error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    deviceclusterid = ReadUint64(p);
    p += sizeof(deviceclusterid);

    // 检查用户是否是群的所有者
    uint64_t clustercreator = m_useroperator->getDeviceClusterOwner(deviceclusterid);
    if (clustercreator != userid)
    {
        // 没权限
        ::writelog("delte cluster no auth");
        msg->appendContent(&ANSC::AUTH_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    // 检查群里面是否有设备, 如果有则不可以删除
    if (m_useroperator->deviceClusterHasDevice(deviceclusterid))
    {
        ::writelog("can not delete cluster when there are device in cluster.");
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    std::string clusteraccount;
    if (m_useroperator->getClusterAccount(deviceclusterid, &clusteraccount) != 0)
    {
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    // 生成通知要取群里面所有的人, 这个群被解散了.
    // 取群里所有的人
    std::list<uint64_t> usersincluster;
    if (m_useroperator->getUserIDInDeviceCluster(deviceclusterid, &usersincluster) < 0)
    {
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    // ok
    if (m_useroperator->deleteDeviceCluster(deviceclusterid) != 0)
    {
        ::writelog("delete cluster failed");
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    // ok
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(deviceclusterid);

    genClusterDelNotify(userid, deviceclusterid, clusteraccount, usersincluster, r);
    return r->size();
}

void DeleteDeviceCluster::genClusterDelNotify(uint64_t userid,
        uint64_t deviceclusterid,
        const std::string &clusteraccount,
        std::list<uint64_t> usersincluster,
        std::list<InternalMessage *> *r)
{
    std::string account;
    if (m_useroperator->getUserAccount(userid, &account) != 0)
        return;

    std::vector<char> notifybuf;
    ::WriteUint64(&notifybuf, deviceclusterid);
    ::WriteString(&notifybuf, clusteraccount);
    ::WriteUint64(&notifybuf, userid);
    ::WriteString(&notifybuf, account);
    Notification notify;
    notify.Init(0, // 通知接收者, 先写0, 在下面循环中修改
        ANSC::SUCCESS,
        CMD::NTP_DCL__DELETE, // 通知类型/通知号
        0, // 通知序列号, 在NotifySrv_d生成
        notifybuf.size(), // 通知内容长度
        &notifybuf[0]);    // 通知内容
    for (auto it = usersincluster.begin(); it != usersincluster.end(); ++it)
    {
        notify.SetObjectID(*it);
        Message *msg = new Message;
        msg->setObjectID(*it); // 通知接收者ID
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);
        msg->appendContent(&notify);

        r->push_back(imsg);
    }
}

UserExitDeviceCluster::UserExitDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int UserExitDeviceCluster::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    // 
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID();
    uint64_t clusterid;

    if (plen < sizeof(clusterid))
    {
        ::writelog("exit device cluster message error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    clusterid = ReadUint64(p);
    p += sizeof(clusterid);

    if (m_useroperator->getDeviceClusterOwner(clusterid) == userid)
    {
        ::writelog(InfoType,
            "cluster owner can not exit cluster: %lu",
            userid);
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }

    if (m_useroperator->deleteUserFromDeviceCluster(userid, clusterid) != 0)
    {
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(clusterid);

    // 通知群里面的成员有人退出了
    genNotifyUserExitCluster(userid, clusterid, r);

    // -用户的设备列表变化, 发送此消息
    // 此消息的处理器UserDeviceListChanged会更新这个用户的场景连接设备
    // 把没有权限的设备删除
    //Message *updatesceneconnector = Message::createClientMessage();
    //updatesceneconnector->SetCommandID(CMD::USER_DEVICE_LIST_CHANGED);
    //updatesceneconnector->SetObjectID(userid);
    //InternalMessage *sceneimsg = new InternalMessage(
    //    updatesceneconnector,
    //    InternalMessage::Internal,
    //    InternalMessage::Unused);
    //MainQueue::postCommand(new NewMessageCommand(sceneimsg));

    // 用户退出群后,那么原来场景里面连接的设备,可能会有一些变得不可用
    // 所以要把这些不可用的设备从场景的连接里面删除掉
    SceneConnectDevicesOperator sceneop(m_useroperator);
    sceneop.findInvalidDevice(userid);

    return r->size();
}

void UserExitDeviceCluster::genNotifyUserExitCluster(uint64_t userid,
        uint64_t clusterid,
        std::list<InternalMessage *> *r)
{
    std::string useraccount;
    std::string clusteraccount;
    if (m_useroperator->getUserAccount(userid, &useraccount) != 0)
        return;
    if (m_useroperator->getClusterAccount(clusterid, &clusteraccount) != 0)
        return;
    std::vector<char> buf;
    Notification notify;
    ::WriteUint64(&buf, clusterid);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, userid);
    ::WriteString(&buf, useraccount);
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::CMD_DCL__EXIT,
        0,
        buf.size(),
        &buf[0]);
    std::list<uint64_t> idlist;
    if (m_useroperator->getUserIDInDeviceCluster(clusterid, &idlist) != 0)
        return;

    for (auto it = idlist.begin(); it != idlist.end(); ++it)
    {
        Message *msg = new Message;
        notify.SetObjectID(*it); // 接收者
        msg->setObjectID(*it); // 接收者
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);

        r->push_back(imsg);
    }
}

//---
DeviceClusterModifyNoteName::DeviceClusterModifyNoteName(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int DeviceClusterModifyNoteName::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint8_t res = ANSC::SUCCESS;
    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());
    responseMsg->setDestID(reqmsg->objectID());

    r->push_back(new InternalMessage(responseMsg));
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("modify notename descrypt error.");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    if (uint64_t(plen) <= sizeof(uint64_t))
    {
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    uint64_t clusterID = ReadUint64(p);
    p += sizeof(clusterID);
    plen -= sizeof(clusterID);

    std::string noteName;
    int rl = readString(p, plen, &noteName);
    if (rl <= 0)
    {
        ::writelog("modify cluster notename: read notename error ");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    if(m_useroperator->modifyClusterNoteName(reqmsg->objectID(),
            clusterID,
            noteName) < 0)
    {
        res = ANSC::FAILED;
    }

    responseMsg->appendContent(&res, Message::SIZE_ANSWER_CODE);
    return r->size();
}

//--
DeviceClusterModifyDescription::DeviceClusterModifyDescription(ICache *c,
                                                               UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{

}

int DeviceClusterModifyDescription::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());
    r->push_back(new InternalMessage(responseMsg));
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("modify cluster desc decrypt failed");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    char *p = (char *)reqmsg->content();
    uint32_t len = reqmsg->contentLen();

    uint64_t clusterID;
    std::string description;
    uint8_t result;

    if (len <= sizeof(clusterID))
    {
        ::writelog("modify cluster desc: error message.");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(p);
    p += sizeof(clusterID);
    len -= sizeof(clusterID);

    int rl = readString(p, len, &description);
    if (rl <= 0)
    {
        ::writelog("modify cluster desc: read desc error.");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    if (m_useroperator->modifyClusterDescription(reqmsg->objectID(),
            clusterID,
            description) == -1)
    {
        result = ANSC::FAILED;
    }
    else
    {
        result = ANSC::SUCCESS;
    }

    responseMsg->appendContent(&result, Message::SIZE_ANSWER_CODE);

    // 生成通知, 通知群里面的成员描述变了
    genClusterDescChangedNotify(clusterID, description, r);
    return r->size();
}

void DeviceClusterModifyDescription::genClusterDescChangedNotify(
        uint64_t clusterID,
        const std::string &description,
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, description);
    Notification notify;
    notify.Init(0,       // 通知接收者, 先写个0, 在下面修改
        ANSC::SUCCESS,   // .
        CMD::CMD_DCL__MODIFY_DISCRIPTION, // 通知号/通知类型
        0, // 通知序列号, 在NotifySrv_d中生成
        buf.size(), //
        &buf[0]);
    std::list<uint64_t> usersincluster;

    IsOnline isonline(m_cache, RedisCache::instance());

    if (m_useroperator->getUserIDInDeviceCluster(clusterID, &usersincluster) != 0)
        return;
    for (auto it = usersincluster.begin(); it != usersincluster.end(); ++it)
    {
        // TODO fixme, 还要在redis中查
        if (!isonline.isOnline(*it))
            continue;

        notify.SetObjectID(*it); // 接收者
        Message *msg = new Message;
        msg->setObjectID(*it); // 接收者
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY); //
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        imsg->setType(InternalMessage::ToNotify);
        r->push_back(imsg);
    }
}

GetClusterListOfUser::GetClusterListOfUser(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int GetClusterListOfUser::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(ANSC::MESSAGE_ERROR);
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);
        // 解密失败
        return r->size();
    }
    if (reqmsg->versionNumber() >= 0x180)
    {
        r->push_back(new InternalMessage(getClusterList_V180(reqmsg)));
        return r->size();
    }
    else
    {
        return 0;
    }
}

Message *GetClusterListOfUser::getClusterList_V180(Message *reqmsg)
{
    uint8_t answerCode = ANSC::SUCCESS;
    char *p = reqmsg->content();
    Message* responseMsg = new Message();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());

    if (reqmsg->contentLen() < 2)
    {
        ::writelog("Error: OnGetClusterList_V2, Message Len error.");
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return responseMsg;
    }
    // 取得用户所在的所有设备群, ObjectID()是当前用户id
    std::vector<DevClusterInfo> vClusterInfo;
    uint16_t startpos = 0;

    startpos = ReadUint16(p);

    // DevClusDBOperator dcOperator(m_sql);
    if(m_useroperator->getClusterListOfUser(reqmsg->objectID(),
            &vClusterInfo) < 0)
    {
        vClusterInfo.clear();
        answerCode = ANSC::FAILED;
    }
    else
    {
        if (startpos > vClusterInfo.size())
        {
            startpos = vClusterInfo.size();
        }
    }

    // 查找所有的群信息
    uint16_t allclustercount = vClusterInfo.size();
    uint16_t clusterCount = 0;

    responseMsg->appendContent(&answerCode, sizeof(answerCode));
    // 总数
    responseMsg->appendContent(allclustercount);
    // 本次响应中包含的数量, 这里先写个0, 后面会修改
    responseMsg->appendContent(clusterCount);

    // max messge size
    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (::getProtoVersion(reqmsg->versionNumber()) >= 0x202)
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }

    for(uint32_t i = startpos; i < vClusterInfo.size(); ++i)
    {
        const DevClusterInfo &ci = vClusterInfo[i];

        uint32_t tmp = responseMsg->contentLen();

        tmp += sizeof(ci.role);
        tmp += sizeof(ci.id);
        tmp = tmp + ci.account.size() + 1;
        tmp = tmp + ci.noteName.size() + 1;
        //tmp = tmp + ci.description.size() + 1;

        if (tmp > maxmsgsize)
        {
            break;
        }

        ++clusterCount;

        responseMsg->appendContent(ci.role);
        responseMsg->appendContent(ci.id);
        responseMsg->appendContent(ci.account);
        responseMsg->appendContent(ci.noteName);
        //responseMsg->appendContent(ci.description);
    }

    // 修改clusterCount
    if (clusterCount > 0)
    {
        uint16_t *p = (uint16_t *)(responseMsg->content() + 1 + 2);
        *p = clusterCount;
    }

    return responseMsg;
}


GetDeviceClusterInfo::GetDeviceClusterInfo(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int GetDeviceClusterInfo::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message* responseMsg = new Message;
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setObjectID(0);
    responseMsg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(responseMsg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        // 解密失败
        return r->size();
    }
    uint64_t clusterID;
    if (reqmsg->contentLen() != sizeof(clusterID))
    {
        responseMsg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(reqmsg->content());
    uint8_t result;

    DevClusterInfo clusterInfo;
    if(m_useroperator->getClusterInfo(reqmsg->objectID(),
            clusterID,
            &clusterInfo) != 0)
    {
        result = ANSC::FAILED;
    }
    else
    {
        result = ANSC::SUCCESS;
    }

    responseMsg->appendContent(result);
    responseMsg->appendContent(&(clusterInfo.role), sizeof(clusterInfo.role));
    responseMsg->appendContent(&(clusterInfo.id), sizeof(clusterInfo.id));
    responseMsg->appendContent(clusterInfo.account);
    responseMsg->appendContent(clusterInfo.noteName);
    responseMsg->appendContent(clusterInfo.description);
    responseMsg->appendContent(uint8_t(0)); // 认证状态, 先不管

    return r->size();
}

//-------取得在群里面的设备

GetDeviceInCluster::GetDeviceInCluster(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{

}

int GetDeviceInCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(reqmsg->objectID());
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);

        msg->appendContent(ANSC::MESSAGE_ERROR);
        // 解密失败
        return r->size();
    }
    if (reqmsg->versionNumber() >= 0x180)
    {
        r->push_back(new InternalMessage(getDeviceInCluster_V180(reqmsg)));
        return r->size();
    }
    else
    {
        return 0;
    }
}

Message *GetDeviceInCluster::getDeviceInCluster_V180(Message *reqmsg)
{
    Message* msg= new Message();
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());

    char *p = reqmsg->content();
    uint8_t answerCode = ANSC::SUCCESS;
    uint64_t clusterID = 0;
    uint16_t startpos = 0;
    std::vector<DeviceInfo> vDeviceInfo;

    if (reqmsg->contentLen() < 8 + 2)
    {
        ::writelog("OnGetDeviceMemberList_V2 message size error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return msg;
    }

    clusterID = ReadUint64(p);
    p += 8;
    startpos = ReadUint16(p);

    if (m_useroperator->getDevMemberInCluster(clusterID, &vDeviceInfo) < 0)
    {
        answerCode = ANSC::FAILED;
        vDeviceInfo.clear();
    }
    else
    {
        if (startpos > vDeviceInfo.size())
        {
            startpos = vDeviceInfo.size();
        }
    }

    uint16_t allcount = vDeviceInfo.size();
    uint16_t protoversion = ::getProtoVersion(reqmsg->versionNumber());

    msg->appendContent(&answerCode, sizeof(answerCode));
    // 群ID
    msg->appendContent(&clusterID, sizeof(clusterID));
    // 总数
    msg->appendContent(allcount);
    // 本次响应的数量, 这里先写个0, 后面会修改
    uint16_t deviceCount = 0;
    msg->appendContent(deviceCount);

    IsOnline isonline(m_cache, RedisCache::instance());
    uint32_t maxmessgesize = MAX_MSG_DATA_SIZE_V2;
    if (protoversion == uint16_t(0x202))
    {
        maxmessgesize = TCP_MAX_MSG_DATA_SIZE; //
    }

    for(uint32_t i=startpos; i < vDeviceInfo.size(); ++i)
    {
        const DeviceInfo &di = vDeviceInfo[i];
        uint32_t tmp = msg->contentLen()
            + Message::SIZE_HEADER_AND_ENDING; // 这里应该用 Contentlen()

        // 在线状态
        uint8_t online;
        tmp += sizeof(di.id);
        tmp += sizeof(online);
        tmp += sizeof(di.macAddr);
        tmp = tmp + di.name.size() + 1;
        tmp = tmp + di.noteName.size() + 1;
        // 协议版本201添加了一具type字段, 占一个字节
        if (protoversion == uint16_t(0x201)
            || protoversion == uint16_t(0x202))
        {
            tmp += sizeof(uint8_t);
        }
        // tmp = tmp + di.description.size() + 1;

        if (tmp > maxmessgesize)
        {
            break;
        }

        ++deviceCount;

        online = isonline.isOnline(di.id) ? ONLINE : OFFLINE;

        msg->appendContent(di.id);
        msg->appendContent(online);
        //char mac[6];
        //uint64ToMacAddr(di.macAddr, mac);
        msg->appendContent(&di.macAddr[0], 6);
        // 协议版本201添加了一具type字段, 占一个字节
        if (protoversion == uint16_t(0x201)
            || protoversion == uint16_t(0x202))
        {
            msg->appendContent(uint8_t(di.type));
        }
        msg->appendContent(di.name);
        //msg->appendContent(di.noteName);
        // msg->appendContent(di.description);
    }

    // 修改数量
    if (deviceCount > 0)
    {
        uint16_t *p = (uint16_t *)(msg->content()
                            + sizeof(answerCode)
                            + sizeof(clusterID)
                            + sizeof(allcount));
        *p = deviceCount;
    }

    return msg;
}

GetOnlineDeviceOfCluster::GetOnlineDeviceOfCluster(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{

}

// TODO fix me 没有分包处理
int GetOnlineDeviceOfCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message* msg = new Message();
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    // CacheManager cacheManager;
    shared_ptr<UserData> userData
        = static_pointer_cast<UserData>(
                            m_cache->getData(reqmsg->objectID()));
    if(!userData)
    {
        return 0;
    }

    // get device in cluster
    char* pos = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    uint64_t clusterID;
    uint16_t startpos;

    if (plen < sizeof(clusterID) + sizeof(startpos))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    startpos = ReadUint16(pos);

    uint16_t all = 0;
    uint16_t deviceCount = 0;
    uint8_t answercode = ANSC::SUCCESS;

    std::vector<DeviceInfo> devices;
    if (m_useroperator->getDevMemberInCluster(clusterID, &devices) < 0)
    {
        answercode = ANSC::FAILED;
        msg->appendContent(&answercode, sizeof(answercode));
        msg->appendContent(&clusterID, sizeof(clusterID));
        msg->appendContent(all);
        msg->appendContent(deviceCount);
        return r->size();
    }
    all = devices.size();
    msg->appendContent(&answercode, sizeof(answercode));
    msg->appendContent(&clusterID, sizeof(clusterID));
    msg->appendContent(all);
    msg->appendContent(&deviceCount, sizeof(deviceCount));

    IsOnline isonline(m_cache, RedisCache::instance());

    for(auto it = startpos; it < devices.size(); ++it)
    {
        const DeviceInfo &di = devices[it];

        uint64_t deviceID = di.id;
        uint8_t onlineStatus;
        if (isonline.isOnline(deviceID))
        {
            deviceCount++;
            onlineStatus = ONLINE;
        }
        else
        {
            onlineStatus = OFFLINE;
            continue;
        }

        msg->appendContent(deviceID);
        msg->appendContent(onlineStatus);
    }

    //std::vector<char> sess = userData->sessionKey();
    //responseMsg->Encrypt(&(sess[0]));
    // 修改总数字段
    if (deviceCount > 0)
    {
        char *p = msg->content();
        uint16_t *count = (uint16_t *)(p
            + sizeof(answercode)
            + sizeof(clusterID))
            + sizeof(all);
        *count = deviceCount;
    }

    return r->size();
}

// 取得群里面某一个设备的所有使用者
GetOperatorListOfDeviceInCluster::GetOperatorListOfDeviceInCluster(ICache *c, UserOperator *uop)
    : m_useroperator(uop), m_cache(c)
{
}

int GetOperatorListOfDeviceInCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("GetOperatorListOfDeviceInCluster decrypt msg failed.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    char* p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t clusterID = 0; // ReadUint64(pos);
    uint64_t deviceMemberID = 0; //ReadUint64(pos += sizeof(clusterID));
    uint16_t startpos = 0;

    if (plen < sizeof(clusterID) + sizeof(deviceMemberID) + sizeof(startpos))
    {
        ::writelog("GetOperatorListOfDeviceInCluster msg error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::vector<DeviceAutho> vDeviceAutho;

    clusterID = ReadUint64(p);
    p += sizeof(clusterID);

    deviceMemberID = ReadUint64(p);
    p += sizeof(deviceMemberID);

    startpos = ReadUint16(p);

    // clusterID 没有用到啊
    if(m_useroperator->getOperatorListOfDevMember(
            clusterID, deviceMemberID, &vDeviceAutho) < 0)
    {
        ::writelog("GetOperatorListOfDeviceInCluster read db failed.");
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    uint16_t allusers = vDeviceAutho.size();
    uint16_t protoversion = ::getProtoVersion(reqmsg->versionNumber());

    msg->appendContent(ANSC::SUCCESS);
    // 群id
    msg->appendContent(&clusterID, sizeof(clusterID));
    // 设备id
    msg->appendContent(&deviceMemberID, sizeof(deviceMemberID));
    // 用户总数
    msg->appendContent(&allusers, sizeof(allusers));

    // 本次应答包含的用户数
    uint16_t userCount = 0;
    msg->appendContent(&userCount, sizeof(userCount));

    if (protoversion == uint16_t(0x201)
        || protoversion == uint16_t(0x202))
    {
        uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
        if (protoversion == uint16_t(0x202))
        {
            maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
        }
        this->writeListToMessageV201(msg,
                                     startpos,
                                     &vDeviceAutho,
                                     &userCount,
                                     maxmsgsize);
    }
    else
    {
        this->writeListToMessageV200(msg, startpos, &vDeviceAutho, &userCount);
    }

    // 修改....这个字段前面是选写了个0占位
    if (userCount > 0)
    {
        uint16_t *p = (uint16_t *)(msg->content()
            + 1  // ansc
            + 8  // 群id字节数
            + 8  // 设备id字节数
            +2); // 总数字节数
        *p = userCount;
    }

    return r->size();
}

void GetOperatorListOfDeviceInCluster::writeListToMessageV200(
        Message *destmsg,
        uint32_t startpos,
        std::vector<DeviceAutho> *deviceAuthos,
        uint16_t *cnt)
{
    std::vector<DeviceAutho> &vDeviceAutho = *deviceAuthos;
    for (uint32_t i = startpos; i < vDeviceAutho.size(); ++i)
    {
        uint32_t tmp = destmsg->contentLen(); // 应该用ContentLen()

        tmp += sizeof(vDeviceAutho[i].id);
        tmp += sizeof(vDeviceAutho[i].authority);
        tmp = tmp + vDeviceAutho[i].name.size() + 1;

        if (tmp > MAX_MSG_DATA_SIZE_V2)
        {
            break;
        }

        *cnt = *cnt + 1;

        destmsg->appendContent(vDeviceAutho[i].id);
        destmsg->appendContent(vDeviceAutho[i].authority);
        destmsg->appendContent(vDeviceAutho[i].name);
    }
}

// 协议201版本和200版本不一样, 加了几个字段, 同时原来的字段位置也变了
// 用户ID 用户帐号 群用户昵称 头像编号 用户在线状态 权限
void GetOperatorListOfDeviceInCluster::writeListToMessageV201(
        Message *destmsg,
        uint32_t startpos,
        std::vector<DeviceAutho> *deviceAuthos,
        uint16_t *cnt,
        uint32_t maxmsgsize)
{
    IsOnline isonline(m_cache, RedisCache::instance());
    std::vector<DeviceAutho> &vDeviceAutho = *deviceAuthos;
    for (uint32_t i = startpos; i < vDeviceAutho.size(); ++i)
    {
        uint32_t tmp = destmsg->contentLen(); // 应该用ContentLen()

        tmp += sizeof(vDeviceAutho[i].id);
        tmp += sizeof(vDeviceAutho[i].authority);
        tmp = tmp + vDeviceAutho[i].name.size() + 1; // 帐号
        // 
        tmp = tmp + vDeviceAutho[i].signature.size() + 1;
        tmp = tmp + sizeof(uint8_t); // head
        tmp = tmp + sizeof(uint8_t); // online statu

        if (tmp > maxmsgsize)
        {
            break;
        }

        *cnt = *cnt + 1;

        destmsg->appendContent(vDeviceAutho[i].id);
        destmsg->appendContent(vDeviceAutho[i].name);
        destmsg->appendContent(vDeviceAutho[i].signature);
        // header 1 byte
        destmsg->appendContent(uint8_t(vDeviceAutho[i].header));
        uint8_t onlinestatu = isonline.isOnline(vDeviceAutho[i].id) ? ONLINE : OFFLINE;
        destmsg->appendContent(onlinestatu);
        destmsg->appendContent(uint8_t(vDeviceAutho[i].authority));
    }

}

//- 取设备群里面用户可用的设备
GetDeviceListOfUserInDeviceCluster::GetDeviceListOfUserInDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetDeviceListOfUserInDeviceCluster::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    char* p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t clusterID = 0;//ReadUint64(pos);
    uint64_t userMemberID = 0; //ReadUint64(pos += sizeof(clusterID));
    uint16_t startpos = 0;
    std::vector<DeviceAutho> vDeviceAutho;
    vDeviceAutho.reserve(100);

    if (plen < sizeof(clusterID) + sizeof(userMemberID) + sizeof(startpos))
    {
        ::writelog("OnGetDeviceListOfUser_V2: Message len error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    clusterID = ReadUint64(p);
    p += sizeof(clusterID);
    userMemberID = ReadUint64(p);
    p += sizeof(userMemberID);
    startpos = ReadUint16(p);

    if(m_useroperator->getDeviceListOfUserMember(
            clusterID, userMemberID, &vDeviceAutho) < 0)
    {
        ::writelog("OnGetDeviceListOfUser_V2: access db failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    if (startpos > vDeviceAutho.size())
    {
        startpos = vDeviceAutho.size();
    }

    uint16_t alldevicecount = vDeviceAutho.size();
    uint16_t protoversion = ::getProtoVersion(reqmsg->versionNumber());
    IsOnline isonline(m_cache, RedisCache::instance());

    msg->appendContent(ANSC::SUCCESS);
    // 群ID
    msg->appendContent(&clusterID, sizeof(clusterID));
    // 用户ID
    msg->appendContent(&userMemberID, sizeof(userMemberID));
    // 总数
    msg->appendContent(&alldevicecount, sizeof(alldevicecount));

    uint16_t deviceCount = 0; // 本次应答包含用设备数, 先写个0, 后面再修改
    msg->appendContent(&deviceCount, sizeof(deviceCount));

    // max msgsize
    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (protoversion >= 0x202)
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }

    ::writelog(InfoType, "Ver: %u", protoversion);
    for(auto it = startpos; it < vDeviceAutho.size(); ++it)
    {
        const DeviceAutho &da = vDeviceAutho[it];

        uint32_t tmp = msg->contentLen(); // 这里应该用Contentlen()

        tmp += sizeof(da.id);
        tmp += sizeof(da.authority);
        tmp = tmp + da.name.size() + 1;
        // 0x201版本, 指令格式, 增加了MAC地址, 设备类型和在线状态三项
        if (protoversion == uint16_t(0x201)
            || protoversion == uint16_t(0x202))
        {
            tmp += 6; // mac 占6字节
            tmp += sizeof(uint8_t); // 类型1字节
            tmp += sizeof(uint8_t); // 在线状态1字节
        }

        if (tmp > maxmsgsize)
        {
            break;
        }

        ++deviceCount;

        msg->appendContent(da.id);
        msg->appendContent(da.authority);
        msg->appendContent(da.name);
        // 0x201版本, 指令格式, 增加了MAC地址, 设备类型和在线状态三项
        if (protoversion == uint16_t(0x201)
                || protoversion == uint16_t(0x202))
        {
            // mac
            msg->appendContent(&da.macAddr[0], 6);
            msg->appendContent(uint8_t(da.type));
            uint8_t onlinestatu = isonline.isOnline(da.id) ? ONLINE : OFFLINE;
            msg->appendContent(onlinestatu);
        }
    }

    // 修改设备数量字段
    if (deviceCount > 0)
    {
        uint16_t *p = (uint16_t *)(msg->content()
            + 1 // answc
            + 8 // clusterid
            + 8 // userid
            + 2); // 总数
        *p = deviceCount;
    }

    return r->size();
}

// 取设备群中的用户成员列表
GetUerMembersInDeviceCluster::GetUerMembersInDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetUerMembersInDeviceCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t clusterID = 0;
    uint16_t startpos = 0;

    if (plen < sizeof(clusterID) + sizeof(startpos))
    {
        ::writelog("OnGetUserMemberList_V2 Error Message Size");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    clusterID = ReadUint64(p);
    p += sizeof(clusterID);
    startpos = ReadUint16(p);

    std::vector<UserMemberInfo> vUserInfo;
    if(m_useroperator->getUserMemberInfoList(clusterID, &vUserInfo) < 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    uint8_t answerCode = ANSC::SUCCESS;
    msg->appendContent(&answerCode, sizeof(answerCode));
    // 群id
    msg->appendContent(&clusterID, sizeof(clusterID));
    // 总数
    uint16_t allusers = vUserInfo.size();
    msg->appendContent(&allusers, sizeof(allusers));
    // 本次应答包含的用户数, 这里先写个0, 后面再修改
    uint16_t userCount = 0;
    msg->appendContent(&userCount, sizeof(userCount));

    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (::getProtoVersion(reqmsg->versionNumber()) == uint32_t(0x202))
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }

    IsOnline isonline(m_cache, RedisCache::instance());

    for(auto iter = startpos; iter < vUserInfo.size(); ++iter)
    {
        const UserMemberInfo &umi = vUserInfo[iter];
        uint32_t tmp = msg->contentLen(); // 这里应该用 Contentlen()
        uint8_t online;
        tmp += sizeof(umi.role);
        tmp += sizeof(umi.id);
        tmp += sizeof(online);
        tmp = tmp + umi.account.size() + 1;
        tmp = tmp + umi.signature.size() + 1;
        tmp = tmp + sizeof(umi.head);

        if (tmp > maxmsgsize)
        {
            break;
        }

        online = isonline.isOnline(umi.id) ? ONLINE : OFFLINE;

        // RedisCache::instance()->
        ++userCount;

        msg->appendContent(umi.role);
        msg->appendContent(umi.id);
        msg->appendContent(online);
        msg->appendContent(umi.account);
        msg->appendContent(umi.signature); // protoversion 0x201 change this to signature
        msg->appendContent(umi.head);
    }
    // 修改用户数, 1和8是字段偏移
    if (userCount > 0)
    {
        uint16_t *p = (uint16_t *)(msg->content()
                            + sizeof(answerCode)
                            + sizeof(clusterID)
                            + sizeof(allusers));
        *p = userCount;
    }

    return r->size();
}

GetUserMemberOnlineStatusInDeviceCluster::GetUserMemberOnlineStatusInDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetUserMemberOnlineStatusInDeviceCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
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

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t clusterID = 0;
    uint16_t startpos = 0;

    if (plen < sizeof(clusterID) + sizeof(startpos))
    {
        ::writelog("OnGetUserMemberList_V2 Error Message Size");
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    clusterID = ReadUint64(p);
    p += sizeof(clusterID);
    startpos = ReadUint16(p);

    std::vector<UserMemberInfo> vUserInfo;
    if(m_useroperator->getUserMemberInfoList(clusterID, &vUserInfo) < 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    uint8_t answerCode = ANSC::SUCCESS;
    msg->appendContent(&answerCode, sizeof(answerCode));
    // 群id
    msg->appendContent(&clusterID, sizeof(clusterID));
    // 总数
    uint16_t allusers = vUserInfo.size();
    msg->appendContent(&allusers, sizeof(allusers));
    // 本次应答包含的用户数, 这里先写个0, 后面再修改
    uint16_t userCount = 0;
    msg->appendContent(&userCount, sizeof(userCount));

    IsOnline isonline(m_cache, RedisCache::instance());

    for(auto iter = startpos; iter < vUserInfo.size(); ++iter)
    {
        const UserMemberInfo &umi = vUserInfo[iter];
        uint32_t tmp = msg->contentLen(); // 这里应该用 Contentlen()
        uint8_t online;
        tmp += sizeof(umi.id);
        tmp += sizeof(online);
        if (tmp > MAX_MSG_DATA_SIZE_V2)
        {
            break;
        }

        online = isonline.isOnline(umi.id) ? ONLINE : OFFLINE;
        ++userCount;

        msg->appendContent(umi.id);
        msg->appendContent(online);
    }
    // 修改用户数, 1和8是字段偏移
    if (userCount > 0)
    {
        uint16_t *p = (uint16_t *)(msg->content()
                            + sizeof(answerCode)
                            + sizeof(clusterID)
                            + sizeof(allusers));
        *p = userCount;
    }

    return r->size();
}

// 设备移交
TransferDeiceToAnotherClusterProcessor::TransferDeiceToAnotherClusterProcessor(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
    m_transfer = new TransferDeviceToAnotherCluster(m_useroperator);
}

int TransferDeiceToAnotherClusterProcessor::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(reqmsg->objectID());
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);

        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    if (reqmsg->commandID() == CMD::CMD_DCL__TRANSFER_DEVICES__REQ)
    {
        bool isFromDevice = GetIdType(reqmsg->objectID()) == IdType::IT_Device;
        if (isFromDevice)
        {
            return deviceClusterChanged(reqmsg, r);
        }
        else
        {
            return transferreq(reqmsg, r);
        }
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__TRANSFER_DEVICES__APPROVAL)
    {
        return transferApprover(reqmsg, r);
    }
    else
    {
        assert(false);
        return 0;
    }
}

int TransferDeiceToAnotherClusterProcessor::deviceClusterChanged(
    Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    // uint64_t srcClusterID;
    //uint64_t destClusterID;
    uint16_t deviceCount;
    uint64_t deviceid;
    if (plen < 32 + 32 + sizeof(deviceCount) + sizeof(deviceid))
    {
        ::writelog("transfer req message error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }


    // TODO .源群直接从数据库中取
    //std::pair<uint64_t, std::string> srcClusterIDAcc;
    // getClusterIDAndAccount(pos, 32, &srcClusterIDAcc);
    pos  += 32;
    plen -= 32;

    std::pair<uint64_t, std::string> destClusterIDAcc;
    bool res2 = getClusterIDAndAccount(pos, 32, &destClusterIDAcc);
    pos += 32;
    plen -= 32;

    if (!res2)
    {
        ::writelog("dev transfer req read cluster id acc failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    //srcClusterID = ReadUint64(pos);
    //plen -= sizeof(srcClusterID);

    //destClusterID = ReadUint64(pos += sizeof(srcClusterID));
    //plen -= sizeof(destClusterID);

    deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);
    plen -= sizeof(deviceCount);

    if (plen != deviceCount * sizeof(uint64_t))
    {
        ::writelog("transfer req message error 2.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    std::vector<uint64_t> vDeviceID;
    vDeviceID.reserve(deviceCount);
    for(uint16_t i = 0; i != deviceCount; i++)
    {
        vDeviceID.push_back(ReadUint64(pos));
        pos += sizeof(uint64_t);
        break; // 只读一个, 因为设备发来的只可以是它己
    }
    uint32_t res = m_transfer->deviceClusterChanged(vDeviceID[0],
                        destClusterIDAcc.first,
                        destClusterIDAcc.second,
                        r);
    msg->appendContent(uint8_t(res));
    if (uint8_t(res) == ANSC::SUCCESS)
    {
        msg->appendContent(reqmsg->content(), 32);
        msg->appendContent(reqmsg->content() + 32, 32);
    }
    return r->size();
}

// buflen一定是32字节
// 如果buf第一个字节0, 则从buf剩下的字节中读8个字节群id
// 如果buf第一个字节不是0, 则从buf中读一个字符串作为群号
bool TransferDeiceToAnotherClusterProcessor::getClusterIDAndAccount(char *buf,
    int buflen,
    std::pair<uint64_t, std::string> *out)
{
    ClusterDal cd;
    assert(buflen == 32);
    uint64_t clusterid;
    std::string clusteraccount;
    bool isClusterid = uint8_t(*buf) == 0;
    if (isClusterid)
    {
        clusterid = ::ReadUint64(buf + 1);
        // 取群号
        if (m_useroperator->getClusterAccount(clusterid, &clusteraccount) != 0)
            return false;
    }
    else
    {
        int readlen = ::readString(buf, buflen, &clusteraccount);
        if (readlen <= 1) // 1 表示一个空字符串
            return false;
        if (cd.getDeviceClusterIDByAccount(clusteraccount, &clusterid) != 0)
            return false;
    }

    out->first = clusterid;
    out->second = clusteraccount;

    return true;
}

int TransferDeiceToAnotherClusterProcessor::transferreq(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t srcClusterID;
    uint64_t destClusterID;
    uint16_t deviceCount;
    if (plen < sizeof(srcClusterID) + sizeof(destClusterID ) + sizeof(deviceCount))
    {
        ::writelog("transfer req message error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    srcClusterID = ReadUint64(pos);
    plen -= sizeof(srcClusterID);

    destClusterID = ReadUint64(pos += sizeof(srcClusterID));
    plen -= sizeof(destClusterID);

    deviceCount = ReadUint16(pos += sizeof(destClusterID));
    pos += sizeof(deviceCount);
    plen -= sizeof(deviceCount);

    if (plen < deviceCount * sizeof(uint64_t))
    {
        ::writelog("transfer req message error 2.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    std::vector<uint64_t> vDeviceID;
    vDeviceID.reserve(deviceCount);
    for(uint16_t i = 0; i != deviceCount; i++)
    {
        vDeviceID.push_back(ReadUint64(pos));
        pos += sizeof(uint64_t);
    }
    uint32_t res = m_transfer->req(vDeviceID,
                    reqmsg->objectID(),
                    srcClusterID,
                    destClusterID,
                    r);
    msg->appendContent(uint8_t(res));
    msg->appendContent(srcClusterID);
    msg->appendContent(destClusterID);

    return r->size();
}

int TransferDeiceToAnotherClusterProcessor::transferApprover(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint8_t result;
    uint64_t transferID;
    uint64_t srcClusterID;
    uint64_t destClusterID;
    uint64_t destClusterOwner = reqmsg->objectID();
    uint16_t count;

    uint32_t leastsize = sizeof(result)
            + sizeof(transferID)
            + sizeof(srcClusterID)
            + sizeof(destClusterID)
            + sizeof(count);
    if (plen < leastsize)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    //char* contentEnd = reqmsg->Content() + reqmsg->ContentLen();

    result = ReadUint8(pos);
    pos += sizeof(result);

    transferID = ReadUint64(pos);
    pos += sizeof(transferID);

    srcClusterID = ReadUint64(pos);
    pos += sizeof(srcClusterID);

    destClusterID = ReadUint64(pos);
    pos += sizeof(destClusterID);

    count = ReadUint16(pos);
    pos += sizeof(count);

    plen -= leastsize;

    if (plen < count * sizeof(uint64_t))
    {
        ::writelog("transfer deivce err msg.2");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::vector<uint64_t> vDeviceID;
    for (uint16_t i = 0; i != count; i++)
    {
        uint64_t deviceID = ReadUint64(pos);
        pos += sizeof(deviceID);

        vDeviceID.push_back(deviceID);
    }

    uint32_t res = m_transfer->approval(result,
                         vDeviceID,
                         transferID,
                         destClusterOwner,
                         srcClusterID,
                         destClusterID,
                         r);

    msg->appendContent(uint8_t(res));

    return r->size();
}


AssignDevicetToUser::AssignDevicetToUser(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int AssignDevicetToUser::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    uint64_t adminID = reqmsg->objectID();
    uint64_t clusterID;
    uint64_t userMemberID;
    uint16_t deviceCount;

    char* pos = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    int headerlen = sizeof(clusterID) + sizeof(userMemberID) + sizeof(deviceCount);
    if (plen < headerlen)
    {
        ::writelog("assdevice to member msg error.1");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    pos += sizeof(clusterID);

    userMemberID = ReadUint64(pos);
    pos += sizeof(userMemberID);

    deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);

    plen -= headerlen;
    if (deviceCount * (sizeof(uint64_t) + sizeof(uint8_t)) > plen)
    {
        ::writelog("assdevice to member msg error.2");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::map<uint64_t, uint8_t> mapDeviceIDAuthority;
    for(uint16_t i = 0; i != deviceCount; i++) {
        uint64_t deviceID = ReadUint64(pos);
        uint8_t authority = ReadUint8(pos += sizeof(deviceID));
        pos += sizeof(authority);

        mapDeviceIDAuthority.insert(std::pair<uint64_t, uint8_t> (deviceID, authority));
    }

    int result = m_useroperator->asignDevicesToUserMember(adminID, clusterID, userMemberID, mapDeviceIDAuthority);
    if(result != 0)
    {
        ::writelog("assign device db operatur failed.");
        msg->appendContent(uint8_t(result)); // ANSC
        return r->size();
    }
    // 回复操作成功
    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(&clusterID, sizeof(clusterID));
    msg->appendContent(&userMemberID, sizeof(userMemberID));

    genNotifyUserGetSomeDevice(userMemberID,
        adminID,
        clusterID,
        mapDeviceIDAuthority,
        r);

    return r->size();
}

void AssignDevicetToUser::genNotifyUserGetSomeDevice(uint64_t userMemberID,
    uint64_t adminid,
    uint64_t clusterID,
    const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority,
    std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string adminAccount;
    m_useroperator->getUserAccount(adminid, &adminAccount);
    m_useroperator->getClusterAccount(clusterID, &clusteraccount);
    std::vector<char> buf;
    ::WriteUint64(&buf, adminid);
    ::WriteString(&buf, adminAccount);
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint16(&buf, uint16_t(mapDeviceIDAuthority.size()));

    for (auto it = mapDeviceIDAuthority.begin();
        it != mapDeviceIDAuthority.end();
        ++it)
    {
        uint64_t devid = it->first;
        std::string devname;
        m_useroperator->getDeviceName(devid, &devname);
        ::WriteUint64(&buf, devid);
        ::WriteString(&buf, devname);
    }
    Notification notify;
    notify.Init(userMemberID,
        ANSC::SUCCESS,
        CMD::CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER,
        0,
        buf.size(),
        &buf[0]);
    Message *msg = new Message;
    msg->setObjectID(userMemberID);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}

RemoveDeviceOfUserMember::RemoveDeviceOfUserMember(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int RemoveDeviceOfUserMember::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    uint64_t adminID = reqmsg->objectID();

    char* pos = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    uint64_t clusterID;
    uint64_t userMemberID;
    uint16_t deviceCount;

    uint16_t headerlen = sizeof(clusterID)
            + sizeof(userMemberID)
            + sizeof(deviceCount);
    if (plen < headerlen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    pos += sizeof(clusterID);

    userMemberID = ReadUint64(pos);
    pos += sizeof(userMemberID);

    deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);

    plen -= headerlen;

    if (deviceCount * (sizeof(uint64_t) + sizeof(uint8_t)) > plen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::map<uint64_t, uint8_t> mapDeviceIDAuthority;
    for (uint16_t i = 0; i != deviceCount; i++)
    {
        uint64_t deviceID = ReadUint64(pos);
        uint8_t authority = ReadUint8(pos += sizeof(deviceID));
        pos += sizeof(authority);

        mapDeviceIDAuthority.insert(std::pair<uint64_t, uint8_t> (deviceID, authority));
    }

    // 
    bool isadmin = false;
    int checkresult = m_useroperator->checkClusterAdmin(adminID, clusterID, &isadmin);
    if (checkresult != 0)
    {
        ::writelog("remove device of user check auth failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    else if (!isadmin)
    {
        ::writelog("remove device of user no auth.");
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }
    int result =  m_useroperator->removeDevicesOfUserMember(adminID,
                                                    clusterID,
                                                    userMemberID,
                                                    mapDeviceIDAuthority);
    if(result < 0)
    {
        ::writelog("remove device of user db operator failed.");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(&clusterID, sizeof(clusterID));
    msg->appendContent(&userMemberID, sizeof(userMemberID));

    genNotifyUserLostDevice(userMemberID,
        adminID,
        clusterID,
        mapDeviceIDAuthority,
        r);

    // -用户的设备列表变化, 发送此消息
    // 此消息的处理器UserDeviceListChanged会更新这个用户的场景连接设备
    // 把没有权限的设备删除
    //Message *updatesceneconnector = Message::createClientMessage();
    //updatesceneconnector->SetCommandID(CMD::USER_DEVICE_LIST_CHANGED);
    //updatesceneconnector->SetObjectID(userMemberID);
    //InternalMessage *sceneimsg = new InternalMessage(
    //    updatesceneconnector,
    //    InternalMessage::Internal,
    //    InternalMessage::Unused);
    //MainQueue::postCommand(new NewMessageCommand(sceneimsg));

    // 设备取消授权后,那么原来场景里面连接的设备,可能会有一些变得不可用
    // 所以要把这些不可用的设备从场景的连接里面删除掉
    SceneConnectDevicesOperator sceneop(m_useroperator);
    sceneop.findInvalidDevice(userMemberID);

    return r->size();
}

void RemoveDeviceOfUserMember::genNotifyUserLostDevice(uint64_t userMemberID,
        uint64_t adminID,
        uint64_t clusterID,
        const std::map<uint64_t, uint8_t> &mapDeviceIDAuthority,
        std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string adminAccount;
    m_useroperator->getUserAccount(adminID, &adminAccount);
    m_useroperator->getClusterAccount(clusterID, &clusteraccount);
    std::vector<char> buf;
    ::WriteUint64(&buf, adminID);
    ::WriteString(&buf, adminAccount);
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint16(&buf, uint16_t(mapDeviceIDAuthority.size()));

    for (auto it = mapDeviceIDAuthority.begin();
        it != mapDeviceIDAuthority.end();
        ++it)
    {
        uint64_t devid = it->first;
        std::string devname;
        m_useroperator->getDeviceName(devid, &devname);
        ::WriteUint64(&buf, devid);
        ::WriteString(&buf, devname);
    }
    Notification notify;
    notify.Init(userMemberID,
        ANSC::SUCCESS,
        CMD::CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER,
        0,
        buf.size(),
        &buf[0]);
    Message *msg = new Message;
    msg->setObjectID(userMemberID);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}

GiveDeviceToManyUser::GiveDeviceToManyUser(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GiveDeviceToManyUser::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    uint64_t adminID = reqmsg->objectID();
    uint64_t clusterID;
    uint64_t deviceID;
    uint16_t userCount;

    char* pos = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    int headerlen = sizeof(clusterID) + sizeof(deviceID) + sizeof(userCount);
    if (plen < headerlen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    pos += sizeof(clusterID);

    deviceID = ReadUint64(pos);
    pos += sizeof(deviceID);

    userCount = ReadUint16(pos);
    pos += sizeof(userCount);

    if (userCount * (sizeof(uint64_t) + sizeof(uint8_t)) > plen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::map<uint64_t, uint8_t> mapUserIDAuthority;
    for (uint16_t i = 0; i != userCount; i++)
    {
        uint64_t userID = ReadUint64(pos);
        uint8_t authority = ReadUint8(pos += sizeof(userID));
        pos += sizeof(authority);

        mapUserIDAuthority.insert(std::pair<uint64_t, uint8_t> (userID, authority));
    }
    // 实质就是操作 user-device 表
    int result = m_useroperator->assignDeviceToManyUser(adminID, clusterID, deviceID, mapUserIDAuthority);
    if(result != 0)
    {
        msg->appendContent(uint8_t(result)); // ansc
        return r->size();
    }
    // 回复成功
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(clusterID);
    msg->appendContent(deviceID);

    genNotifyUserGetDevice(adminID, clusterID, deviceID, mapUserIDAuthority, r);
    return r->size();
}

void GiveDeviceToManyUser::genNotifyUserGetDevice(
    uint64_t adminID,
    uint64_t clusterID,
    uint64_t deviceID,
    const std::map<uint64_t, uint8_t> &mapUserIDAuthority,
    std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string adminAccount;
    std::string devname;
    m_useroperator->getUserAccount(adminID, &adminAccount);
    m_useroperator->getClusterAccount(clusterID, &clusteraccount);
    m_useroperator->getDeviceName(deviceID, &devname);
    std::vector<char> buf;
    ::WriteUint64(&buf, adminID);
    ::WriteString(&buf, adminAccount);
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, deviceID);
    ::WriteString(&buf, devname);

    Notification notify;
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER,
        0,
        buf.size(),
        &buf[0]);
    for (auto it = mapUserIDAuthority.begin();
        it != mapUserIDAuthority.end();
        ++it)
    {
        notify.SetObjectID(it->first);
        Message *msg = new Message;
        msg->setObjectID(it->first);
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);

        r->push_back(imsg);
    }
}

//
RemoveDeviceUsers::RemoveDeviceUsers(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int RemoveDeviceUsers::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    uint64_t adminID = reqmsg->objectID();

    char* pos = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();

    uint64_t clusterID;
    uint64_t deviceID;
    uint16_t userCount;

    int headerlen = sizeof(clusterID) + sizeof(deviceID) + sizeof(userCount);
    if (plen < headerlen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    pos += sizeof(clusterID );

    deviceID = ReadUint64(pos);
    pos += sizeof(deviceID );

    userCount = ReadUint16(pos);
    pos += sizeof(userCount);

    plen -= headerlen;

    if (userCount * (sizeof(uint64_t) + sizeof(uint8_t)) > plen)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::map<uint64_t, uint8_t> mapUserIDAuthority;
    for (uint16_t i = 0; i != userCount; i++)
    {
        uint64_t userID = ReadUint64(pos);
        uint8_t authority = ReadUint8(pos += sizeof(userID));
        pos += sizeof(authority);

        mapUserIDAuthority.insert(std::make_pair(userID, authority));
    }

    int result= m_useroperator->removeDeviceUsers(adminID, clusterID, deviceID, mapUserIDAuthority);
    if(result != 0)
    {
        msg->appendContent(uint8_t(result)); // ansc
        return r->size();
    }

    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(clusterID);
    msg->appendContent(deviceID);

    genNotifyUserLostDevice(adminID, clusterID, deviceID, mapUserIDAuthority, r);

    // 设备取消授权后,那么原来场景里面连接的设备,可能会有一些变得不可用
    // 所以要把这些不可用的设备从场景的连接里面删除掉
    SceneConnectDevicesOperator sceneop(m_useroperator);
    for (auto uid : mapUserIDAuthority)
    {
        sceneop.findInvalidDevice(uid.first);
    }

    return r->size();
}

void RemoveDeviceUsers::genNotifyUserLostDevice(
    uint64_t adminID,
    uint64_t clusterID,
    uint64_t deviceID,
    const std::map<uint64_t, uint8_t> &mapUserIDAuthority,
    std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    std::string devname;
    std::string adminaccount;
    m_useroperator->getUserAccount(adminID, &adminaccount);
    m_useroperator->getClusterAccount(clusterID, &clusteraccount);
    m_useroperator->getDeviceName(deviceID, &devname);
    std::vector<char> buf;
    ::WriteUint64(&buf, adminID);
    ::WriteString(&buf, adminaccount);
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, deviceID);
    ::WriteString(&buf, devname);

    Notification notify;
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER,
        0,
        buf.size(),
        &buf[0]);
    for (auto it = mapUserIDAuthority.begin();
        it != mapUserIDAuthority.end();
        ++it)
    {
        notify.SetObjectID(it->first);
        Message *msg = new Message;
        msg->setObjectID(it->first);
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);

        r->push_back(imsg);
    }
}

//----
InviteUserToDeviceCluster::InviteUserToDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int InviteUserToDeviceCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(reqmsg->objectID());
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);

        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__INVITE_TO_JOIN__REQ)
    {
        return inviteReq(reqmsg, r);
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL)
    {
        return inviteApproval(reqmsg, r);
    }
    else
    {
        assert(false);
        return 0;
    }
}

int InviteUserToDeviceCluster::inviteReq(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t clusterID;
    uint64_t inviteeID;

    if (plen < sizeof(clusterID) + sizeof(inviteeID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    inviteeID = ReadUint64(pos += sizeof(clusterID));

    //检查管理员权限
    bool isAdmin = false;
    if(m_useroperator->checkClusterAdmin(reqmsg->objectID(), clusterID, &isAdmin) < 0)
    {
        ::writelog("invite to join db operator failed.");
    }

    if(!isAdmin)//不是管理员，无权限
    {
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    //获取群号
    std::string clusterAccount;
    if(m_useroperator->getClusterAccount(clusterID, &clusterAccount) != 0)
    {
        ::writelog("invite to join get cluster account failed.");
        msg->appendContent(ANSC::CLUSTER_ID_ERROR);
        return r->size();
    }

    //获取管理员信息
    UserInfo *adminInfo;
    adminInfo = m_useroperator->getUserInfo(reqmsg->objectID());
    std::unique_ptr<UserInfo> _del(adminInfo);
    if(adminInfo == 0)
    {
        ::writelog("invite to join get user info failed.");
        msg->setDestID(reqmsg->objectID());
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    msg->appendContent(ANSC::SUCCESS);

    // 生成通知, 通知被邀请者
    genNotifyToTargetUser(reqmsg->objectID(),
        adminInfo->account,
        clusterID, 
        clusterAccount,
        inviteeID,
         r);

    return r->size();
}

int InviteUserToDeviceCluster::inviteApproval(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint8_t approvalResult;
    uint64_t clusterID;
    uint64_t adminID;
    uint64_t inviteeID = reqmsg->objectID(); // 被邀请者ID

    if (plen < sizeof(approvalResult) + sizeof(clusterID) + sizeof(adminID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    approvalResult = ReadUint8(pos);
    clusterID = ReadUint64(pos += sizeof(approvalResult));
    adminID = ReadUint64(pos += sizeof(clusterID));

    std::string clusterAccount;
    if(m_useroperator->getClusterAccount(clusterID, &clusterAccount) != 0)
    {
        // yes no return;
    }

    std::string inviteeaccount;
    if (m_useroperator->getUserAccount(reqmsg->objectID(), &inviteeaccount) != 0)
    {
        msg->appendContent(ANSC::USER_ID_OR_DEVICE_ID_ERROR);
        return r->size();
    }

    if (approvalResult == ANSC::REFUSE)
    {
        // cout << "invitee refused this application" << endl;
        ::writelog("invitee refused this application");
        genNotifyAdminUserAccept(ANSC::REFUSE, // 拒绝了
            adminID,
            clusterID,
            clusterAccount,
            inviteeID, // 被邀请者ID
            inviteeaccount, // 被邀请者帐号
            r);

        msg->appendContent(ANSC::SUCCESS);
        return r->size();
    }

    bool isAdmin = false;
    if(m_useroperator->checkClusterAdmin(adminID, clusterID, &isAdmin) < 0)
    {
        ::writelog("invite user join cluster check cluster admin db operator failed.");
        // return;
    }

    if(!isAdmin)
    {
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    UserDao ud;
    // 检查是否超过限制, 最多创建(拥用)100个群
    if  (!ud.isClusterLimitValid(reqmsg->objectID()))
    {
        ::writelog(InfoType, "user cluster max limit.%lu", reqmsg->objectID());
        msg->appendContent(ANSC::CLUSTER_MAX_LIMIT);
        return r->size();
    }

    // 检查是否超过限制
    ClusterDal cd;
    if (!cd.canAddUser(clusterID))
    {
        ::writelog(InfoType, "invite can add user failed.%lu", clusterID);
        msg->appendContent(ANSC::CLUSTER_MAX_USER_LIMIT);
        return r->size();
    }

    //if(approvalResult == ANSC::ANSC_AGREE) {
    if(m_useroperator->insertUserMember(reqmsg->objectID(), clusterID) != 0)
    {
        ::writelog("invite user join cluster insert user member db operator failed.");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    else
    {
        msg->appendContent(ANSC::SUCCESS);
    }
    //}

    genNotifyAdminUserAccept(ANSC::ACCEPT, // 是否同意
        adminID,
        clusterID,
        clusterAccount,
        inviteeID, // 被邀请者ID
        inviteeaccount, // 被邀请者帐号
        r);

    genNotifyNewUserAddedToCluster(clusterID,
        clusterAccount,
        inviteeID, // 加入者ID
        inviteeaccount, // 加入者帐号
        r);
    //uint32_t notifNumber = 0;
    //msg->SetCommandID(CMD::CMD_GEN__NOTIFY);
    //msg->SetObjectID(adminID);
    //msg->SetDestID(adminID);
    //msg->appendContent(&ANSC::Success, Message::SIZE_ANSWER_CODE);
    //msg->appendContent(&notifNumber, Message::SIZE_NOTIF_NUMBER);
    //msg->appendContent(&CMD::NTP_DCL__INVITE_TO_JOIN__RESULT, Message::SIZE_NOTIF_TYPE);
    //msg->appendContent(&approvalResult, sizeof(approvalResult));
    //msg->appendContent(&clusterID, sizeof(clusterID));
    //msg->appendContent(clusterAccount);
    //msg->appendContent(&(inviteeInfo->userid), sizeof(inviteeInfo->userid));
    //msg->appendContent(inviteeInfo->account);

    return r->size();
}

void InviteUserToDeviceCluster::genNotifyToTargetUser(uint64_t clusteradmin, // 邀请者ID
        const std::string &adminaccount,// 邀请者帐号
        uint64_t clusterID, // 群ID
        const std::string &clusterAccount, // 群帐号
        uint64_t inviteeID, // 被邀请者ID
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusterAccount);
    ::WriteUint64(&buf, clusteradmin);
    ::WriteString(&buf, adminaccount);
    Notification notify;
    notify.Init(inviteeID, // 接收者, 被邀请者
        ANSC::SUCCESS, // .
        CMD::CMD_DCL__INVITE_TO_JOIN__REQ, // 通知号/通知类型
        0, // 序列号
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(inviteeID);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
}

void InviteUserToDeviceCluster::genNotifyAdminUserAccept(uint8_t rst, // 是否同意
        uint64_t adminID,
        uint64_t clusterID,
        const std::string &clusterAccount,
        uint64_t inviteeID, // 被邀请者ID
        const std::string &inviteeaccount, // 被邀请者帐号
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    Notification notify;
    ::WriteUint8(&buf, rst);
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusterAccount);
    ::WriteUint64(&buf, inviteeID); // 被邀请者ID
    ::WriteString(&buf, inviteeaccount);
    notify.Init(adminID,
                ANSC::SUCCESS,
                CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL,
                0,
                buf.size(),
                &buf[0]);
    Message *msg = new Message;
    msg->setObjectID(adminID); // 接收者
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    r->push_back(imsg);
}

void InviteUserToDeviceCluster::genNotifyNewUserAddedToCluster(uint64_t clusterID,
        const std::string &clusterAccount,
        uint64_t inviteeID, // 加入者ID
        const std::string &inviteeaccount, // 加入者帐号
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusterAccount);
    ::WriteUint64(&buf, inviteeID);
    ::WriteString(&buf, inviteeaccount);
    Notification notify;
    notify.Init(0, // 接收者ID, 在后面改
        ANSC::SUCCESS,
        CMD::NOTIFY_NEW_ADDED_TO_CLUSTER,
        0, // 序列号
        buf.size(),
        &buf[0]);
    std::list<uint64_t> usersincluster;
    if (m_useroperator->getUserIDInDeviceCluster(clusterID, &usersincluster) != 0)
        return;

    for (auto it = usersincluster.begin(); it != usersincluster.end(); ++it)
    {
        //if (*it == inviteeID)
            //continue;

        notify.SetObjectID(*it); // 接收者ID
        Message *msg = new Message;
        msg->setObjectID(*it); // 接收者ID
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        imsg->setType(InternalMessage::ToNotify);

        r->push_back(imsg);
    }
}

RemoveUserFromDeviceCluster::RemoveUserFromDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{

}

int RemoveUserFromDeviceCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    uint64_t adminID = reqmsg->objectID();

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t clusterID;
    uint16_t userMemberCount;
    if (plen < sizeof(clusterID) + sizeof(userMemberCount))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    pos += sizeof(clusterID);
    plen -= sizeof(clusterID);

    userMemberCount = ReadUint16(pos);
    pos += sizeof(userMemberCount);
    plen -= sizeof(userMemberCount);

    if (plen < userMemberCount * sizeof(uint64_t))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    // m_useroperator->checkClusterAdmin(adminID, isadmin);

    uint64_t clusterowner;
    clusterowner = m_useroperator->getDeviceClusterOwner(clusterID);

    uint64_t userMemberID;
    std::vector<uint64_t> vUserMemberID;
    vUserMemberID.reserve(100);
    for(uint16_t i = 0; i != userMemberCount; i++)
    {
        userMemberID = ReadUint64(pos);
        pos += sizeof(userMemberID);
        // 不可以删除自己, 也不可以删除拥用者
        if(userMemberID == adminID || userMemberID == clusterowner)
        {
            continue;
        }
        vUserMemberID.push_back(userMemberID);
    }

    // 这个函数会检查adminID是否有权限
    int result = m_useroperator->removeUserMembers(adminID, vUserMemberID, clusterID);

    if(result != 0)
    {
        ::writelog("remove user from cluster failed.");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // 回复操作成功
    msg->setDestID(adminID);
    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(&clusterID, sizeof(clusterID));
    msg->appendContent(uint16_t(vUserMemberID.size()));
    for (auto it = vUserMemberID.begin(); it != vUserMemberID.end(); ++it)
    {
        msg->appendContent(*it);
    }

    // 通知被删除的人.
    std::string adminAccount;
    if(0 != m_useroperator->getUserAccount(adminID, &adminAccount))
    {
        ::writelog("remove user from cluster : get user account failed");
        // yes no return;
    }

    std::string clusterAccount;
    if(0 != m_useroperator->getClusterAccount(clusterID, &clusterAccount))
    {
        ::writelog("remove user from cluster : get cluster account failed");
        // yes no return;
    }

    // 通知被删除的用户
    genNotifyUserBeRemoved(clusterID,
        clusterAccount,
        adminID,
        adminAccount,
        vUserMemberID,
        r);
    // 通知群里面的用户哪些人被删除了
    genNotifySomeUserRemovedFromCluster(clusterID,
        clusterAccount,
        vUserMemberID,
        r);

    // -用户的设备列表变化, 发送此消息
    // 此消息的处理器UserDeviceListChanged会更新这个用户的场景连接设备
    // 把没有权限的设备删除
    //
    SceneConnectDevicesOperator sceneop(m_useroperator);
    for (auto uid : vUserMemberID)
    {
        //Message *updatesceneconnector = Message::createClientMessage();
        //updatesceneconnector->SetCommandID(CMD::USER_DEVICE_LIST_CHANGED);
        //updatesceneconnector->SetObjectID(uid);
        //InternalMessage *sceneimsg = new InternalMessage(
        //    updatesceneconnector,
        //    InternalMessage::Internal,
        //    InternalMessage::Unused);
        //MainQueue::postCommand(new NewMessageCommand(sceneimsg));
        // 用户退出群后,那么原来场景里面连接的设备,可能会有一些变得不可用
        // 所以要把这些不可用的设备从场景的连接里面删除掉

        sceneop.findInvalidDevice(uid);
    }

    return r->size();
}
// 通知被删除的用户
void RemoveUserFromDeviceCluster::genNotifyUserBeRemoved(uint64_t clusterID,
        const std::string &clusterAccount,
        uint64_t adminid,
        const std::string &adminaccount,
        const std::vector<uint64_t> &vUserMemberID,
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    Notification notify;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusterAccount);
    ::WriteUint64(&buf, adminid);
    ::WriteString(&buf, adminaccount);
    notify.Init(0, // 接收者ID, 后面再改
        ANSC::SUCCESS,
        CMD::CMD_DCL__REMOVE_USER_MEMBERS, // .
        0, // 序列号, 在别的地方生成, 先写0
        buf.size(),
        &buf[0]);

    for (auto it = vUserMemberID.begin(); it != vUserMemberID.end(); ++it)
    {
        notify.SetObjectID(*it); // 接收者
        Message *msg = new Message;
        msg->setObjectID(*it); // 接收者
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);

        r->push_back(imsg);
    }
}

// 通知群里面的用户哪些人被删除了
void RemoveUserFromDeviceCluster::genNotifySomeUserRemovedFromCluster(uint64_t clusterID,
        const std::string &clusterAccount,
        const std::vector<uint64_t> &vUserMemberID,
        std::list<InternalMessage *> *r)
{
    std::list<uint64_t> idlist;
    if (m_useroperator->getUserIDInDeviceCluster(clusterID, &idlist) != 0)
        return;

    for (auto vit = vUserMemberID.begin(); vit != vUserMemberID.end(); ++vit)
    {
        std::string account;
        if (m_useroperator->getUserAccount(*vit, &account) != 0)
            continue;
        std::vector<char> buf;
        ::WriteUint64(&buf, clusterID);
        ::WriteString(&buf, clusterAccount);
        ::WriteUint64(&buf, *vit); // 被删除的人
        ::WriteString(&buf, account);
        Notification notify;
        notify.Init(0, // 接收者, 先写0, 后面再改
            ANSC::SUCCESS,
            CMD::CMD_A_USER_BE_REMOVED_FROM_CLUSTER, // 通知类型/通知号
            0,
            buf.size(),
            &buf[0]);
        for (auto it = idlist.begin(); it != idlist.end(); ++it)
        {
            uint64_t receiver = *it;
            notify.SetObjectID(receiver);
            Message *msg = new Message;
            msg->setObjectID(receiver);
            msg->setCommandID(CMD::CMD_NEW_NOTIFY);
            msg->appendContent(&notify);
            InternalMessage *imsg = new InternalMessage(msg);
            imsg->setType(InternalMessage::ToNotify);
            imsg->setEncrypt(false);

            r->push_back(imsg);
        }
    }
}

ModifyUserMemberRole::ModifyUserMemberRole(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int ModifyUserMemberRole::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t clusterID;
    uint8_t newRole;
    uint64_t userMemberID;

    if (plen < sizeof(clusterID) + sizeof(newRole) + sizeof(userMemberID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    clusterID = ReadUint64(pos);
    newRole = ReadUint8(pos += sizeof(clusterID));
    userMemberID = ReadUint64(pos+= sizeof(newRole));

    if(newRole < 2 || newRole > 3)
    {
        // cout << "wrong old role" << endl;
        // return;
        ::writelog("set user member role error role");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    // 群所有者的权限不可以改
    uint64_t clusterowner = m_useroperator->getDeviceClusterOwner(clusterID);
    if (clusterowner != 0 && clusterowner == userMemberID)
    {
        msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // 这个函数里同会检查objectid是不是有管理员权限
    if(m_useroperator->modifyUserMemberRole(reqmsg->objectID(),
                                    userMemberID,
                                    newRole,
                                    clusterID) < 0)
    {
        ::writelog("set user member role db operator failed");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);

    genNotifyRoleChanged(userMemberID, clusterID, newRole, r);

    return r->size();
}

void ModifyUserMemberRole::genNotifyRoleChanged(uint64_t userid,
        uint64_t clusterID,
        uint8_t newRole,
        std::list<InternalMessage *> *r)
{
    std::string clusteraccount;
    if (m_useroperator->getClusterAccount(clusterID, &clusteraccount) != 0)
        return;
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint8(&buf, newRole);
    Notification notify;
    notify.Init(userid,
        ANSC::SUCCESS,
        CMD::CMD_DCL__MODIFY_USER_MEMBER_ROLE,
        0, // 序列号, 在别的地方生成
        buf.size(),
        &buf[0]);
    Message *msg = new Message;
    msg->setObjectID(userid);
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
}

GetDeviceInfoInCluster::GetDeviceInfoInCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetDeviceInfoInCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    uint64_t deviceID = ReadUint64(reqmsg->content());

    DeviceInfo *deviceInfo;
    deviceInfo = m_useroperator->getDeviceInfo(deviceID);
    std::unique_ptr<DeviceInfo> _del(deviceInfo);
    if(deviceInfo == 0)
    {
        msg->appendContent(&ANSC::FAILED, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(uint8_t(0)); // 组号是个什么东西?? 先写个0
    msg->appendContent(&deviceID, sizeof(deviceID));
    //std::vector<char> &mac = deviceInfo->getMacAddr();
    msg->appendContent(&deviceInfo->macAddr[0], 6);
    msg->appendContent(deviceInfo->name);

    return r->size();
}
// ---
UserApplyToJoinCluster::UserApplyToJoinCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int UserApplyToJoinCluster::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(0);
        msg->setDestID(reqmsg->objectID());
        InternalMessage *imsg = new InternalMessage(msg);
        r->push_back(imsg);
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, (Message::SIZE_ANSWER_CODE));
        return r->size();
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__JOIN__REQ)
    {
        return req(reqmsg, r);
    }
    else if (reqmsg->commandID() == CMD::CMD_DCL__JOIN__APPROVAL)
    {
        return approval(reqmsg, r);
    }
    else
    {
        assert(false);
        return 0;
    }
}

int  UserApplyToJoinCluster::req(
    Message *reqmsg,  std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    InternalMessage *imsg = new InternalMessage(msg);
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    r->push_back(imsg);

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t clusterID;
    if (plen < sizeof(clusterID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    clusterID = ReadUint64(p);

    //获取群号
    string clusterAccount;
    if(m_useroperator->getClusterAccount(clusterID, &clusterAccount) != 0)
    {
        ::writelog("join cluster get cluster account failed.");
        msg->appendContent(ANSC::CLUSTER_ID_ERROR);
        return r->size();
    }

    //获取请求者信息
    UserInfo *userInfo;
    userInfo = m_useroperator->getUserInfo(reqmsg->objectID());
    std::unique_ptr<UserInfo> _ui(userInfo); // for auto delete
    if(userInfo == 0)
    {
        ::writelog("join cluster get user info failed.");
        msg->appendContent(ANSC::CLUSTER_ID_OR_USER_ID_ERROR);
        return r->size();
    }

    //获取管理员ID
    uint64_t sysAdminID;
    if(m_useroperator->getSysAdminID(clusterID, &sysAdminID) != 0)
    {
        ::writelog("join cluster get admin id of clusteer failed.");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    ::writelog(InfoType, "---join: clusterid: %lu, %s, %lu", clusterID, clusterAccount.c_str(), sysAdminID);
    // 回复客户端收到请求
    //
    UserDao ud;
    if (!ud.isClusterLimitValid(reqmsg->objectID()))
    {
        msg->appendContent(ANSC::CLUSTER_MAX_LIMIT);
        return r->size();
    }

    msg->appendContent(ANSC::SUCCESS);

    // 生成一个通知给群拥有者: 有人申请加入
    genNotifyClusterOwner(sysAdminID,
        clusterID,
        clusterAccount,
        reqmsg->objectID(),
        userInfo->account,
        r);
    return r->size();
}

int  UserApplyToJoinCluster::approval(
    Message *reqmsg,  std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    InternalMessage *imsg = new InternalMessage(msg);
    msg->setObjectID(0);
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    r->push_back(imsg);

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t adminID = reqmsg->objectID();
    uint8_t approvalResult;
    uint64_t clusterID;
    uint64_t applicantID;

    if (plen < sizeof(approvalResult) + sizeof(clusterID) + sizeof(applicantID))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    approvalResult = ReadUint8(pos);
    clusterID = ReadUint64(pos += sizeof(approvalResult));
    applicantID = ReadUint64(pos += sizeof(clusterID));

    DevClusterInfo clusterInfo;
    if(m_useroperator->getClusterInfo(applicantID, clusterID, &clusterInfo) != 0)
    {
        // yes no return;
    }

    if (approvalResult == ANSC::REFUSE)
    {
        ::writelog("manager refused this application");
        msg->appendContent(ANSC::SUCCESS);

        // 通知请求者被拒绝了.
        genNotifyToUserResult(applicantID,
            clusterID,
            clusterInfo.account,
            ANSC::REFUSE, // 表示没有同意
            r);
        return r->size();
    }

    bool isAdmin = false;
    if(m_useroperator->checkClusterAdmin(adminID, clusterID, &isAdmin) < 0)
    {
        ::writelog("join cluster check cluster admin failed");
        // return;
    }
    if(!isAdmin)
    {
        ::writelog("join cluster no auth");
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // 检查是否超过限制
    ClusterDal cd;
    if (!cd.canAddUser(clusterID))
    {
        msg->appendContent(ANSC::CLUSTER_MAX_USER_LIMIT);
        return r->size();
    }

    if(m_useroperator->insertUserMember(applicantID, clusterID) < 0)
    {
        ::writelog("join cluster failed");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // ok
    msg->appendContent(ANSC::SUCCESS);

    std::string adminAccount;
    if(m_useroperator->getUserAccount(adminID, &adminAccount) != 0)
    {
        // yes no return;
    }

    // 通知请求者同意了.
    genNotifyToUserResult(applicantID,
        clusterID,
        clusterInfo.account,
        ANSC::ACCEPT, // 表示同意
        r);

    // 通知群里面的人, 有人加入了
    genNotifyToUserInCluster(clusterID, clusterInfo.account, applicantID, r);

    return r->size();
}

void UserApplyToJoinCluster::genNotifyClusterOwner(uint64_t sysAdminID,
        uint64_t clusterID,
        const std::string &clusterAccount,
        uint64_t userid,
        const std::string &useraccount,
        std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusterAccount);
    ::WriteUint64(&buf, userid);
    ::WriteString(&buf, useraccount);
    Notification notify;
    notify.Init(sysAdminID, // 接收者
        ANSC::SUCCESS,  //.
        CMD::NTP_DCL__JOIN__REQ, // 通知号/通知类型
        0, // 序列号, 在别的地方生成
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(sysAdminID); // 接收者
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
}

void UserApplyToJoinCluster::genNotifyToUserResult(uint64_t userid,
            uint64_t clusterid,
            const std::string &clusteraccount,
            uint8_t result,
            std::list<InternalMessage *> *r)
{
    std::vector<char> buf;
    ::WriteUint8(&buf, result); // 申批结果
    ::WriteUint64(&buf, clusterid);
    ::WriteString(&buf, clusteraccount);
    Notification notify;
    notify.Init(userid, // 接收者
        ANSC::SUCCESS,  // .
        CMD::CMD_DCL__JOIN__APPROVAL, // 通知号, 通知类型
        0, // 通知序列号在别的地访生成
        buf.size(),
        &buf[0]);

    Message *msg = new Message;
    msg->setObjectID(userid); // 接收者
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);
    r->push_back(imsg);
}

void UserApplyToJoinCluster::genNotifyToUserInCluster(uint64_t clusterID,
    const std::string &clusteraccount,
    uint64_t applicantID,
    std::list<InternalMessage *> *r)
{
    std::string useraccount;
    if (m_useroperator->getUserAccount(applicantID, &useraccount) != 0)
        return;
    std::vector<char> buf;
    ::WriteUint64(&buf, clusterID);
    ::WriteString(&buf, clusteraccount);
    ::WriteUint64(&buf, applicantID);
    ::WriteString(&buf, useraccount);
    Notification notify;
    notify.Init(0, // 接收者ID, 在后面改
        ANSC::SUCCESS,
        CMD::NOTIFY_NEW_ADDED_TO_CLUSTER,
        0, // 序列号
        buf.size(),
        &buf[0]);
    std::list<uint64_t> usersincluster;
    if (m_useroperator->getUserIDInDeviceCluster(clusterID, &usersincluster) != 0)
        return;

    for (auto it = usersincluster.begin(); it != usersincluster.end(); ++it)
    {
        if (*it == applicantID)
            continue;

        notify.SetObjectID(*it); // 接收者ID
        Message *msg = new Message;
        msg->setObjectID(*it); // 接收者ID
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        msg->appendContent(&notify);
        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        imsg->setType(InternalMessage::ToNotify);

        r->push_back(imsg);
    }
}

//-----------
SearchDeviceCluster::SearchDeviceCluster(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int SearchDeviceCluster::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);
    r->push_back(imsg);

    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        ::writelog("search cluster decrypt failed.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    std::string clusterAccount;
    int rlen = readString(reqmsg->content(), reqmsg->contentLen(), &clusterAccount);
    if (rlen <= 0)
    {
        ::writelog("search cluster message error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    ::writelog(InfoType, "Search cluster: %s", clusterAccount.c_str());
    DevClusterInfo clusterInfo;
    int result = m_useroperator->searchCluster(clusterAccount, &clusterInfo);
    if(result != 0)
    {
        ::writelog("db search failed.");
        msg->appendContent(ANSC::ACCOUNT_NOT_EXIST);
        return r->size();
    }

    clusterInfo.role = 0x00;
    uint16_t clusterCount = 0x0001;

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(&clusterCount, sizeof(clusterCount));
    msg->appendContent(clusterInfo.id);
    msg->appendContent(clusterInfo.account);
    msg->appendContent(clusterInfo.description);

    return r->size();
}
