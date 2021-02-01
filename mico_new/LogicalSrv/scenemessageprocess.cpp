#include <sys/time.h>
#include "scenemessageprocess.h"
#include "messageencrypt.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "Crypt/MD5.h"
#include "Message/Notification.h"
#include "serverinfo/serverinfo.h"
#include "config/configreader.h"
#include "fileuploadredis.h"
#include "sessionkeygen.h"
#include "Message/relaymessage.h"
#include "onlineinfo/devicedata.h"
#include "onlineinfo/rediscache.h"
#include "onlineinfo/isonline.h"
#include "sceneConnectDevicesOperator.h"
#include "dbaccess/scene/scenedal.h"
#include "protocoldef/protocolversion.h"
#include "dbaccess/scene/idelsceneuploadlog.h"
#include "timer/timer.h"
#include "server.h"

GetMyHmiSenceListProcessor::GetMyHmiSenceListProcessor(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetMyHmiSenceListProcessor::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
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
        ::writelog("get scene list decrypt failed.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    uint8_t answerCode = ANSC::SUCCESS;
    char *d = reqmsg->content();
    // 2是因为列表起始位置占2字节, 至少2字节
    if (2 > reqmsg->contentLen())
    {
        ::writelog("Get OnGetMySceneList_V2 : request messqge length error");
        answerCode = ANSC::MESSAGE_ERROR;
        msg->appendContent(answerCode);
        return r->size();
    }

    std::vector<SceneInfo> vSceneInfo;
    vSceneInfo.reserve(200);
    uint16_t startpos = 0;

    startpos = ReadUint16(d);

    if(m_useroperator->getMySceneList(reqmsg->objectID(), &vSceneInfo) < 0)
    {
        answerCode = ANSC::FAILED;
        vSceneInfo.clear();
    }

    if (startpos > vSceneInfo.size())
    {
        startpos = vSceneInfo.size();
    }

    uint16_t allsceneCount = vSceneInfo.size();

    uint16_t sceneCount = 0;
    msg->appendContent(answerCode);
    // 场景总数2字节
    msg->appendContent(&allsceneCount, sizeof(allsceneCount));
    // 本次请求返回的总数, 这里先写0, 后面会修改
    msg->appendContent(&sceneCount, sizeof(sceneCount));

    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (::getProtoVersion(reqmsg->versionNumber()) >= uint32_t(0x202))
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }
    for(uint32_t i = startpos; i < vSceneInfo.size(); i++)
    {
        uint32_t tmp = msg->contentLen();
        SceneInfo &si = vSceneInfo[i];

        tmp += sizeof(si.id);
        //tmp += sizeof(vSceneInfo[i].authority);
        tmp = tmp + si.notename.size() + 1;

        if (tmp > maxmsgsize)
        {
            break;
        }

        ++sceneCount;
        msg->appendContent(&(si.id), sizeof(si.id));
        //msg->appendContent(&(vSceneInfo[i].authority), sizeof(vSceneInfo[i].authority));
        msg->appendContent(si.notename);
    }

    // 修改总数
    if (sceneCount != 0)
    {
        uint16_t *p = (uint16_t *)(msg->content() + 1 + 2);
        *p = sceneCount;
    }

    return r->size();
}

ModifySceneInfo::ModifySceneInfo(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int ModifySceneInfo::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
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
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // TODO FIXME 这里要判断长度是否合法
    // uint8_t answccode;
    std::string version;
    std::string desc;
    uint64_t userid;
    uint64_t sceneid;

    char *p = reqmsg->content();
    int clen = reqmsg->contentLen();

    userid = reqmsg->objectID();

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    clen -= sizeof(sceneid);

    // version
    int readlen = readString(p, clen, &version);
    if (readlen <= 0)
    {
        // error
        ::writelog("mofidy scene info: read version failed.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    clen -= readlen;
    p += readlen;

    // .
    readlen = readString(p, clen, &desc);
    if (readlen <= 0)
    {
        // error
        ::writelog("mofidy scene info: read desc failed.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // TODO FIXME 验证权限
    // modifySceneInfo会忽略掉version参数, 因为场景version会在场景上传时自动增加
    if (m_useroperator->modifySceneInfo(userid, sceneid,
        std::string(""), version, desc) < 0)
    {
        // error
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // ok
    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    return r->size();
}

// modify noate name of scene
ModifyNotename::ModifyNotename(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int ModifyNotename::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
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
        ::writelog("modify scene notename decrypt failed.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    char *p = reqmsg->content();
    int len = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    if (uint64_t(len) <= sizeof(sceneid))
    {
        ::writelog("modify sene notename msg length error.");
        // error
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid );
    len -= sizeof(sceneid);

    std::string notename;
    int readlen = readString(p, len, &notename);
    if (readlen <= 0)
    {
        // error
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    if (m_useroperator->modifySceneNoteName(userid, sceneid, notename) < 0)
    {
        // error
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // ok
    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    return r->size();
}

// 取得一个场景的一些信息
GetHmiSceneInfo::GetHmiSceneInfo(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetHmiSceneInfo::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
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
        ::writelog("get scene info decrypt failed");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    if (uint64_t(plen) < sizeof(sceneid))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    sceneid = ReadUint64(p);
    p += sizeof(sceneid);

    SceneInfo si;
    if (m_useroperator->getSceneInfo(userid, sceneid, &si) < 0)
    {
        msg->appendContent(&ANSC::SCENE_ID_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // .
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(si.id);
    msg->appendContent(si.authority); // userid对场景的使用权限
    msg->appendContent(si.notename);
    msg->appendContent(si.name);
    msg->appendContent(si.version);
    msg->appendContent(si.description);
    msg->appendContent(si.ownerid);
    msg->appendContent(si.ownerusername);

    return r->size();
}

// 
GetHmiSenceShareUsers::GetHmiSenceShareUsers(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{

}

int GetHmiSenceShareUsers::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
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
        // 解密失败
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    uint16_t startpos;

    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    if (uint64_t(plen) < sizeof(sceneid) + sizeof(startpos))
    {
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);

    startpos = ReadUint16(p);
    p += sizeof(startpos);

    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }

    std::vector<SharedSceneUserInfo> users;
    users.reserve(100);
    if (m_useroperator->getSceneSharedUses(userid, sceneid, &users) != 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    uint16_t allsize = users.size();
    uint16_t returnsize = 0;
    uint32_t msgsize = 0;

    uint16_t protoversion = ::getProtoVersion(reqmsg->versionNumber());
    IsOnline isonline(this->m_cache, RedisCache::instance());
    // answc code
    msg->appendContent(ANSC::SUCCESS);
    // all
    msg->appendContent(&allsize, sizeof(allsize));
    // return count, 先写0, 后面再修改
    msg->appendContent(&returnsize, sizeof(returnsize));

    uint32_t maxmsgsize = MAX_MSG_DATA_SIZE_V2;
    if (protoversion >= 0x202)
    {
        maxmsgsize = TCP_MAX_MSG_DATA_SIZE;
    }

    for (uint64_t i = startpos; i < users.size(); ++i)
    {
        // 用户id
        // 权限
        // 用户名
        if (msgsize > maxmsgsize)
        {
            break;
        }
        msgsize += sizeof(uint64_t); // userid
        msgsize += sizeof(uint8_t); // auth
        msgsize += sizeof(uint8_t); // header 
        msgsize += users[i].username.size() + 1;
        msgsize += users[i].signature.size() + 1;
        // 协议201版本添加了一个用户在线状态字节
        if (protoversion == uint16_t(0x201)
                || protoversion == uint16_t(0x202))
        {
            msgsize += sizeof(uint8_t);
        }
        returnsize++;
        msg->appendContent(&users[i].userid, sizeof(uint64_t));
        msg->appendContent(users[i].username);
        msg->appendContent(users[i].signature); // protoversion 201 change this to signature
        msg->appendContent(users[i].header);
        // is user online
        // 协议201版本添加了一个用户在线状态字节
        if (protoversion == uint16_t(0x201)
                || protoversion == uint16_t(0x202))
        {
            uint8_t onlinestatu = isonline.isOnline(users[i].userid) ? ONLINE : OFFLINE;
            msg->appendContent(onlinestatu);
        }
        msg->appendContent(&users[i].authority, sizeof(uint8_t));
    }

    // 修改returnsize字段
    if (returnsize > 0)
    {
        char *p = msg->content();
        p = p + sizeof(uint8_t) // answc code
            + sizeof(allsize);
        uint16_t *ptoretsize = (uint16_t *)p;
        *ptoretsize = returnsize;
    }

    return r->size();
}

//------------------------------------
ShareScene::ShareScene(ICache *c, ISceneDal *sd, UserOperator *uop)
    : m_scenedal(sd), m_cache(c), m_useroperator(uop)
{

}

int ShareScene::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("share scene decrypt failed.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();
    // userid, sceneid, 个数两字节, 用户id, 权限1字节
    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    uint8_t type; //这个没用
    uint8_t auth;
    uint64_t targetuser;
    std::vector< std::pair<uint64_t, uint8_t> > targetusers; // 用户,权限
    if (uint32_t(plen) < sizeof(sceneid) + sizeof(type) + sizeof(targetuser) + sizeof(auth))
    {
        ::writelog("share scene error msg.");
        // 数据错误
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return  r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);

    type = ReadUint8(p);
    p += sizeof(type);
    plen -= sizeof(type);

    targetuser = ReadUint64(p);
    p += sizeof(targetuser);
    plen -= sizeof(targetuser);

    auth = ReadUint8(p);
    p += sizeof(auth);
    plen -= sizeof(auth);

    // 检查权限, user必需是场景的creatorID
    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        ::writelog("share scene no auth.");
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // 检查共享次数是否超过
    int stimes = m_scenedal->getSharedTimes(sceneid);
    int maxtimes = m_scenedal->getMaxShareTimes();
    if (stimes < 0 || maxtimes < 0 || stimes >= maxtimes)
    {
        ::writelog(InfoType, "share scene is at max times: %lu", sceneid);
        msg->appendContent(ANSC::SCENE_SHARE_MAX_LIMIT);
        return r->size();
    }

    targetusers.push_back(std::make_pair(targetuser, auth));

    if (m_useroperator->shareSceneToUser(userid, sceneid, targetusers) != 0)
    {
        ::writelog("shared scene failed.");
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);

    // generate notify ...
    ShareScene::generateNotify(userid,
        targetuser,
        sceneid,
        auth,
        CMD::CMD_HMI__SHARE_SCENE,
        m_useroperator, r);
    return r->size();
}

void ShareScene::generateNotify(uint64_t userid,
    uint64_t targetuserid,
    uint64_t sceneid,
    uint8_t auth,
    uint16_t notifytype,
    UserOperator *useroperator,
    std::list<InternalMessage *> *r)
{
    std::vector<char> notifybuf;
    // 场景所有者ID和account
    std::string account;
    if (useroperator->getUserAccount(userid, &account) != 0)
    {
        ::writelog(InfoType, "share scene gen notify: getuser account failed: %lu", userid);
        return;
    }
    // ID
    ::WriteUint64(&notifybuf, userid);
    // 帐号
    char *p = (char *)account.c_str();
    notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因以字符串要以0结尾

    // 场景ID和场景名
    ::WriteUint64(&notifybuf, sceneid);
    // 场景名
    std::string scenename;
    if (useroperator->getSceneName(sceneid, &scenename) != 0)
    {
        ::writelog(InfoType, "Share scene get scene name failed: %lu", sceneid);
    }
    ::WriteString(&notifybuf, scenename);
    ::WriteUint8(&notifybuf, auth);

    Notification notify;
    notify.Init(targetuserid,
        ANSC::SUCCESS,
        notifytype,
        0,
        notifybuf.size(),
        &notifybuf[0]);

    // 生成通知发到通知处理进程
    Message *msg = new Message;
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->setObjectID(targetuserid); // 通知接收者的ID
    msg->appendContent(&notify);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(false);
    imsg->setType(InternalMessage::ToNotify); // 表示这个消息是要发到通知进程的

    r->push_back(imsg);
}

DonotShareScene::DonotShareScene(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int DonotShareScene::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
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
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID(); 
    uint64_t sceneid;
    uint8_t type;
    uint64_t targetuserid;

    if (plen < sizeof(targetuserid) + sizeof(sceneid) + sizeof(type))
    {
        ::writelog("message error.1");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);

    type = ReadUint16(p);
    p += sizeof(type);
    plen -= sizeof(type);

    targetuserid = ReadUint64(p);
    p += sizeof(targetuserid);
    plen -= sizeof(targetuserid);

    // 验证权限
    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        // 
        ::writelog(InfoType, "noshare auth, failed.user: %lu, scene:%lu", userid, sceneid);
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    std::vector<std::pair<uint64_t, uint8_t> > targetusers;
    targetusers.push_back(std::make_pair(targetuserid, 0));

    if (m_useroperator->cancelShareSceneToUser(userid,
            sceneid, targetusers) != 0)
    {
        ::writelog("cancelShareSceneToUser failed.");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);

    //generateCancelShareNotify(userid, targetuserid, sceneid, r);
    ShareScene::generateNotify(userid,
        targetuserid,
        sceneid,
        0,
        CMD::CMD_HMI__CANCEL_SHARE_SCENE,
        m_useroperator, r);
    return r->size();
}

//void DonotShareScene::generateCancelShareNotify(uint64_t userid,
//        uint64_t targetuser,
//        uint64_t sceneid,
//        std::list<InternalMessage *> *r)
//{
//    Notification notify;
//    notify.SetObjectID(targetuser); // 通知接收者
//    notify.setAnswcCode(ANSC::Success);
//    notify.setNotifyType(CMD::CMD_HMI__CANCEL_SHARE_SCENE);
//
//    std::vector<char> notifybuf;
//    // 场景所有者ID和帐号
//    ::WriteUint64(&notifybuf, userid);
//    std::string account;
//    if (m_useroperator->getUserAccount(userid, &account) != 0)
//        return;
//    char *p = (char *)account.c_str();
//    notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因为字符串以0结尾
//
//    // 场景ID和场景名
//    ::WriteUint64(&notifybuf, sceneid);
//    std::string scenename;
//    if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
//        return;
//    p = (char *)scenename.c_str();
//    notifybuf.insert(notifybuf.end(), p, p + scenename.size() + 1); // 加1是因为字符串以0结尾
//
//    notify.SetMsg(&notifybuf[0], notifybuf.size());
//
//    Message *msg = new Message;
//    msg->SetObjectID(targetuser); // 通知接收者ID
//    msg->SetCommandID(CMD::CMD_NewNotify);
//    InternalMessage *imsg = new InternalMessage(msg);
//    imsg->setEncrypt(false);
//    imsg->setType(InternalMessage::ToNotify);
//
//    r->push_back(imsg);
//}

//--
SetShareSceneAuth::SetShareSceneAuth(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{

}

int SetShareSceneAuth::processMesage(Message *reqmsg,
    std::list<InternalMessage *> *r)
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
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    uint64_t targetuserid;
    uint8_t type; // 这个没用
    uint8_t auth;
    if (plen < sizeof(sceneid) + sizeof(targetuserid) + sizeof(auth) + sizeof(type))
    {
        ::writelog("set scehe auth: message error.1");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // 
    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);

    //
    type = ReadUint8(p);
    p += sizeof(type);
    plen -= sizeof(type);

    //
    targetuserid = ReadUint64(p);
    p += sizeof(targetuserid);
    plen -= sizeof(targetuserid);

    auth  = ReadUint8(p);
    p += sizeof(auth);
    plen -= sizeof(auth);

    // 检查权限
    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        ::writelog("set scene no auth.");
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    if (plen < sizeof(userid) * plen)
    {
        ::writelog("set scene auth: message error.2");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    std::vector<uint64_t> targetusers;
    targetusers.push_back(targetuserid);

    if (m_useroperator->setShareSceneAuth(
            userid, sceneid, auth, targetusers) != 0)
    {
        ::writelog("set scehe auth: setShareSceneAuth failed.2");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);

    //genSceneAuthChangeNotify(userid, targetuserid, sceneid, auth, r);
    ShareScene::generateNotify(userid,
        targetuserid,
        sceneid,
        auth,
        CMD::CMD_HMI__SET_SHARE_SCENE_AUTH,
        m_useroperator,
        r);
    return r->size();
}

//void SetShareSceneAuth::genSceneAuthChangeNotify(uint64_t userid, uint64_t targetuser,
//    uint64_t sceneid, uint8_t auth, std::list<InternalMessage *> *r)
//{
//    Notification notify;
//    notify.SetObjectID(targetuser);// 通知接收者ID
//    notify.setNotifyType(CMD::CMD_HMI__SET_SHARE_SCENE_AUTH);
//    notify.setAnswcCode(ANSC::Success);
//
//    std::vector<char> notifybuf;
//    // 场景所有者ID和帐号
//    ::WriteUint64(&notifybuf, userid);
//    std::string account;
//    if (m_useroperator->getUserAccount(userid, &account) != 0)
//    {
//        return;
//    }
//    char *p = (char *)account.c_str();
//    notifybuf.insert(notifybuf.end(), p, p + account.size() + 1); // 加1是因为字符串以0结尾
//
//    // 场景ID和场景名
//    WriteUint64(&notifybuf, sceneid);
//    std::string scenename;
//    if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
//        return;
//    p = (char *)scenename.c_str();
//    notifybuf.insert(notifybuf.end(), p, p + scenename.size() + 1);
//
//    // 权限
//    WriteUint8(&notifybuf, auth);
//
//    notify.SetMsg(&notifybuf[0], notifybuf.size());
//
//    Message *msg = new Message;
//    msg->SetObjectID(targetuser); // 通知接收者ID
//    msg->appendContent(&notify);
//    msg->SetCommandID(CMD::CMD_NewNotify);
//    InternalMessage *imsg = new InternalMessage(msg);
//    imsg->setType(InternalMessage::ToNotify);
//    imsg->setEncrypt(false);
//
//    r->push_back(imsg);
//}

DeleteScene::DeleteScene(ICache *c, UserOperator *uop ,RunningSceneDbaccess *rdb)
    : m_cache(c), m_useroperator(uop), m_rsdbaccess(rdb) 
{
}

int DeleteScene::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setObjectID(0);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if (::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    if (plen < sizeof(sceneid))
    {
        ::writelog("delete scene msg error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);

	RunningSceneData rd;
	::writelog(InfoType, "get run scene msg111 %llu", rd.runningserverid);
    //RunningSceneDbaccess m_rsdbaccess(m_useroperator->getmysqlconnpool());
	m_rsdbaccess->getRunningScene(sceneid, &rd);
    if (rd.valid() && rd.runningserverid != 0)
    {
        ::writelog("delete scene with alarm error");
        msg->appendContent(&ANSC::DEL_SCENE_WITH_ALARM, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    // 检查权限
    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        ::writelog("delete scene no auth");
        msg->appendContent(&ANSC::AUTH_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    std::vector<SharedSceneUserInfo> sui;
    m_useroperator->getSceneSharedUses(userid, sceneid, &sui);

    uint32_t serverid;
    if (m_useroperator->getSceneServerid(sceneid, &serverid) != 0)
    {
        ::writelog("delete scene failed: get scene serverid failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    std::string scenename;
    if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
        ::writelog("delscene get scene name failed.");

    // post a message to file server
    Configure *cfg = Configure::getConfig();
    std::shared_ptr<ServerInfo> server = ServerInfo::getFileServerInfo(serverid);
    if (!server)
    {
        ::writelog(InfoType, "delscene get serverinfo failed of id: %d", serverid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    std::string ip = server->ip();
    FileUpAndDownloadRedis fr = FileUpAndDownloadRedis::getInstance(ip,
        cfg->file_redis_port,
        cfg->redis_pwd);
    std::string delscenemsg(std::to_string(sceneid));
    delscenemsg.append(":");
    delscenemsg.append(std::to_string(userid));
    fr.postDelSceneMessage(delscenemsg);

    uint32_t scenesize;
    if (m_useroperator->getSceneSize(sceneid, &scenesize) != 0)
    {
        ::writelog("del scene get scenesize failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    if (m_useroperator->deleteScene(sceneid) != 0)
    {
        ::writelog("delete scene failed");
        msg->appendContent(&ANSC::FAILED, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    if (m_useroperator->freeDisk(userid, scenesize) != 0)
    {
        ::writelog(ErrorType, "del scene free diskfailed: %lu, scene: %lu", userid, sceneid);
    }

    // 发消息到文件服务器从文件服务器删除文件

    msg->appendContent(&ANSC::SUCCESS, Message::SIZE_ANSWER_CODE);
    msg->appendContent(sceneid);

    genSceneDelNotify(userid, sceneid, scenename, &sui, r);
    return r->size();
}

void DeleteScene::genSceneDelNotify(uint64_t userid,
    uint64_t sceneid,
    const std::string &scenename,
    std::vector<SharedSceneUserInfo> *sui,
    std::list<InternalMessage *> *r)
{
    if (sui->size() <= 0)
        return;

    std::string account;
    if (m_useroperator->getUserAccount(userid, &account) != 0)
    {
        ::writelog(InfoType, "delscene gen notify get useraccount failed:%lu", userid);
        return;
    }

    std::vector<char> notifybuf;
    // 场景所有者id和帐号
    WriteUint64(&notifybuf, userid);
    ::WriteString(&notifybuf, account);
    // 场景id和帐号
    WriteUint64(&notifybuf, sceneid);
    ::WriteString(&notifybuf, scenename);

    Notification notify;
    notify.Init(0, // 接收者, 先写0, 下面会修改
        ANSC::SUCCESS,
        CMD::CMD_HMI__DELETE_SCENE, // 通知类型/通知号
        0, // 通知序列号, 在NotifySrv中生成
        notifybuf.size(),
        &notifybuf[0]);

    for (auto it = sui->begin(); it != sui->end(); ++it)
    {
        notify.SetObjectID(it->userid); // 接收者id
        Message *msg = new Message;
        msg->setObjectID(it->userid); // 接收者id
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->appendContent(&notify);

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setType(InternalMessage::ToNotify);
        imsg->setEncrypt(false);

        r->push_back(imsg);
    }
}

// 穿透
HmiPenetrate::HmiPenetrate(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{

}

int HmiPenetrate::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t sceneID;
    uint16_t deviceCount;

    if (plen < sizeof(sceneID) + sizeof(deviceCount))
    {
        ::writelog("hmi penetrate error msg");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // ??? 看起来和文档上的格式不一样啊...
    uint64_t userid = reqmsg->objectID();
    sceneID = ReadUint64(pos + (reqmsg->contentLen() - sizeof(sceneID)));
    plen -= sizeof(sceneID);

    deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);
    plen -= sizeof(deviceCount);

    if (plen < deviceCount * sizeof(uint64_t))
    {
        ::writelog("hmi penetrate error msg.2");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }


    // cout << "deviceCount: " << deviceCount << endl;
    std::vector<uint64_t> vDeviceID;
    for(uint16_t i = 0; i < deviceCount; i++)
    {
        uint64_t deviceID = ReadUint64(pos);
        pos += sizeof(uint64_t);
        plen -= sizeof(deviceID);

        vDeviceID.push_back(deviceID);
    }
    // cout << "sceneID: " << ReadUint64(pos) << endl;

    bool hasUseRight = false;
    // 用户对场景是否有权限
    if(m_useroperator->isUserHasAccessRightOfScene(sceneID, userid, &hasUseRight) < 0)
    {
        ::writelog("hmi penetrate check user right failed.");
    }

    if(!hasUseRight)
    {
        ::writelog("hmi penetrate user no auth.");
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }
    // 验证权限, 用户对设备是否有权限, 这个不用验证了
    //if (isUserHassAllDeviceAccessRight(userid, vDeviceID))
    //{
    //    ::writelog("hmi penetrate user no device auth.");
    //    msg->appendContent(ANSC::AuthErr);
    //    return r->size();
    //}
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(deviceCount);

    uint8_t authority = 1;
    for(uint16_t i = 0; i != deviceCount; i++)
    {
        uint64_t deviceID = vDeviceID[i];//ReadUint64(pos);
        // authority = ReadUint8(pos += sizeof(deviceID));
        //pos += sizeof(authority);
        //cout << "---device id: 0x" << hex << deviceID << dec << endl;

        uint32_t deviceIP = 0;
        uint16_t devicePort = 0;
        char deviceSessionKey[16] = {0};
        char CDSessionKey[16] = {0};
        sockaddrwrap deviceSockAddr;

        shared_ptr<DeviceData>  deviceData
            = static_pointer_cast<DeviceData>(m_cache->getData(deviceID));
        //if(authority != 0 && !deviceData.Empty()) 

        if (!deviceData)
        {
            deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(deviceID));
        }

        uint8_t onlineStatus;
        if (deviceData)
        {
            onlineStatus = ONLINE;

            //deviceSockAddr = deviceData.GetSockAddr();
            deviceSockAddr = deviceData->sockAddr();
            sockaddr *addr = deviceSockAddr.getAddress();
            if (addr->sa_family == AF_INET)
            {
                sockaddr_in *in4 = (sockaddr_in *)addr;
                deviceIP = in4->sin_addr.s_addr;
                devicePort = in4->sin_port;
            }
            else if (addr->sa_family == AF_INET6)
            {
                // !!IMP
                assert(false);
                deviceIP = 0;
                devicePort = 0;
            }
            else
            {
                assert(false);
            }

            //memcpy(deviceSessionKey, deviceData.GetCurSessionKey(), 16);
            std::vector<char> sesk = deviceData->GetCurSessionKey();
            assert(sesk.size() == 16);
            memcpy(deviceSessionKey, &(sesk[0]), 16);

            MD5 md5;
            char md5UserID[16] = {0};
            md5.input((const char*) &userid, sizeof(userid));
            md5.output(md5UserID);
            for(size_t i = 0; i != 16; i++)
            {
                CDSessionKey[i] = deviceSessionKey[i] ^ md5UserID[i];
            }
        }
        else
        {
            onlineStatus = OFFLINE;
        }

        uint8_t encryptionMethod = 0x01;
        msg->appendContent(&deviceID, sizeof(uint64_t)); // 随机数, 这里直接写个devid代替
        msg->appendContent(&deviceID, sizeof(deviceID));
        msg->appendContent(&onlineStatus, sizeof(onlineStatus));
        msg->appendContent(&authority, sizeof(authority));
        msg->appendContent(&deviceIP, sizeof(deviceIP));
        msg->appendContent(&devicePort, sizeof(devicePort));
        msg->appendContent(CDSessionKey, 16);
        msg->appendContent(&encryptionMethod, sizeof(encryptionMethod));

        if (authority != 0 && onlineStatus == ONLINE)
        { // ???
            uint8_t  prefix = 0xFF;
            uint8_t  opcode = 0xB0;
            uint32_t clientIP = 0;//reqmsg->sockerAddress().sin_addr.s_addr;
            uint16_t clientPort = 0;//reqmsg->sockerAddress().sin_port;
            sockaddrwrap &addrwrap = reqmsg->sockerAddress();
            sockaddr *addr = addrwrap.getAddress();
            if (addr->sa_family == AF_INET)
            {
                sockaddr_in *in4 = (sockaddr_in *)addr;
                clientIP = in4->sin_addr.s_addr;
                clientPort = in4->sin_port;
            }
            else if (addr->sa_family == AF_INET6)
            {
                // ????
                // !!IMP
            }
            else
            {
                assert(false);
            }

            uint8_t clientUserID = reqmsg->objectID();
            Message* deviceMsg = Message::createDeviceMessage(); // 把客户端地址致电设备
            deviceMsg->setSockerAddress(deviceSockAddr);
            deviceMsg->setCommandID(CMD::DCMD__OLD_FUNCTION);
            deviceMsg->setDestID(deviceID);
            deviceMsg->appendContent(&prefix, sizeof(prefix));
            deviceMsg->appendContent(&opcode, sizeof(opcode));
            deviceMsg->appendContent(&clientIP, sizeof(clientIP));
            deviceMsg->appendContent(&clientPort, sizeof(clientPort));
            deviceMsg->appendContent(&clientUserID, sizeof(clientUserID));

            r->push_back(new InternalMessage(deviceMsg));
        }
    }

    //responseMsg->Encrypt(userData.GetSessionKey());
    //std::vector<char> usrsess = userData->sessionKey();
    //responseMsg->Encrypt(&(usrsess[0]));

    return r->size();
}

HmiRelay::HmiRelay(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int HmiRelay::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    char* pos = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t sceneID;
    uint16_t deviceCount;

    if (plen < sizeof(sceneID) + sizeof(deviceCount))
    {
        ::writelog("hmi relay error msg");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    // ??? 看起来和文档上的格式不一样啊...
    uint64_t userid = reqmsg->objectID();
    sceneID = ReadUint64(pos + (reqmsg->contentLen() - sizeof(sceneID)));
    plen -= sizeof(sceneID);

    deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);
    plen -= sizeof(deviceCount);

    if (plen < deviceCount * sizeof(uint64_t))
    {
        ::writelog("hmi relay error msg.2");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }


    // cout << "deviceCount: " << deviceCount << endl;
    std::vector<uint64_t> vDeviceID;
    for(uint16_t i = 0; i < deviceCount; i++)
    {
        uint64_t deviceID = ReadUint64(pos);
        pos += sizeof(uint64_t);
        plen -= sizeof(deviceID);

        vDeviceID.push_back(deviceID);
    }
    bool hasUseRight = false;
    // 用户对场景是否有权限
    if (sceneID == 0)
    {
        hasUseRight = true;
    }
    else if(m_useroperator->isUserHasAccessRightOfScene(sceneID, userid, &hasUseRight) < 0)
    {
        ::writelog("hmi relay check user right failed.");
    }

    if(!hasUseRight)
    {
        ::writelog("hmi relay user no auth.");
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }
    msg->setCommandID(CMD::CMD_HMI__OPEN_RELAY_CHANNELS);
    //msg->appendContent(Relay::OpenChannel);// 1表示打开通道
    //msg->appendContent(userid);
    //msg->appendContent(deviceCount);

    RelayMessage relaymsg;
    relaymsg.setCommandCode(Relay::OPEN_CHANNEL);
    relaymsg.setUserid(userid);
    relaymsg.setUserSockaddr(reqmsg->sockerAddress());
    // 因为设备不验证权限, 所以写个1
    // TODO Fixme 验证权限
    uint8_t authority = 1;
    for(uint16_t i = 0; i != deviceCount; i++)
    {
        uint64_t deviceID = vDeviceID[i];

        sockaddrwrap deviceSockAddr;
        bzero(&deviceSockAddr, sizeof(deviceSockAddr));

        std::shared_ptr<DeviceData> deviceData
            = static_pointer_cast<DeviceData>(m_cache->getData(deviceID));
        if (!deviceData)
        {
            deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(deviceID));
        }

        if(deviceData)
        {
            deviceSockAddr = deviceData->sockAddr();
        }
        else
        {
            continue;
        }

        RelayDevice rd;
        rd.auth = authority;
        rd.devicdid = deviceID;
        rd.devicesockaddr = deviceSockAddr;

        relaymsg.appendRelayDevice(rd);
    }

    msg->appendContent(&relaymsg);

    imsg->setType(InternalMessage::ToRelay);
    imsg->setEncrypt(false);

    return  r->size();
}

HmiOpenRelayResult::HmiOpenRelayResult(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int HmiOpenRelayResult::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();(void)plen;

    RelayMessage rmsg;
    if (!rmsg.fromByteArray(p, plen))
    {
        return 0;
    }

    const std::list<RelayDevice> &rdlist = rmsg.relaydevice();
    // uint8_t funccode;
    uint64_t userid = rmsg.userid();
    uint16_t devcount = rdlist.size();

    Message *clientmsg = Message::createClientMessage();
    clientmsg->CopyHeader(reqmsg);
    clientmsg->setCommandID(CMD::CMD_HMI__OPEN_RELAY_CHANNELS);
    clientmsg->setDestID(userid);
    clientmsg->setObjectID(0);

    uint8_t answerCode = ANSC::SUCCESS;
    clientmsg->appendContent(answerCode);
    clientmsg->appendContent(devcount);

    sockaddrwrap &addrwrap = rmsg.relayServeraddr();
    sockaddr *addr = addrwrap.getAddress();
    uint32_t ip = 0;
    if (addr->sa_family == AF_INET)
    {
        ip = ((sockaddr_in *)addr)->sin_addr.s_addr;
    }
    else
    {
        // !!IMP
        assert(false);
    }
    for (auto it = rdlist.begin(); it != rdlist.end(); ++it)
    {
        const RelayDevice &rd = *it;
        shared_ptr<DeviceData> deviceData
                = static_pointer_cast<DeviceData>(m_cache->getData(rd.devicdid));
        if (!deviceData)
        {
            deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(rd.devicdid));
        }

        uint8_t onlineStatus = deviceData ? ONLINE : OFFLINE;
        uint8_t authority = 1;
        clientmsg->appendContent(rd.userrandomNumber);
        ::writelog(InfoType, "rd: user:%lu rdnunber: %lu, %lu", userid, rd.userrandomNumber, rd.devrandomNumber);
        clientmsg->appendContent(rd.devicdid);
        clientmsg->appendContent(onlineStatus);
        clientmsg->appendContent(authority);
        clientmsg->appendContent(ip);
        clientmsg->appendContent(htons(rd.portforclient)); // htons

        uint8_t  prefix = 0xFF;
        uint8_t  opcode = 0xB0;
        char deviceSessionKey[16] = {0};
        char CDSessionKey[16] = {0};
        sockaddrwrap deviceSockAddr;
        if(authority != 0 && onlineStatus == ONLINE && deviceData)
        {
            //deviceSockAddr = deviceData.GetSockAddr();
            deviceSockAddr = deviceData->sockAddr();
            Message *deviceMsg = Message::createDeviceMessage();
            deviceMsg->setSockerAddress(deviceSockAddr);
            deviceMsg->setCommandID(CMD::DCMD__OLD_FUNCTION);
            deviceMsg->setObjectID(0);
            deviceMsg->setDestID(rd.devicdid);
            deviceMsg->appendContent(&prefix, sizeof(prefix));
            deviceMsg->appendContent(&opcode, sizeof(opcode));
            deviceMsg->appendContent(ip);
            deviceMsg->appendContent(htons(rd.portfordevice)); // 要进行大小尾转换
            //deviceMsg->appendContent(userid);
            deviceMsg->appendContent(rd.devrandomNumber);

            std::vector<char> sess = deviceData->GetCurSessionKey();
            assert(sess.size() == 16);

            memcpy(deviceSessionKey, &(sess[0]), sess.size());

            MD5 md5;
            char md5UserID[16] = {0};
            md5.input((const char*) &userid, sizeof(userid));
            md5.output(md5UserID);
            for(size_t i = 0; i != 16; i++)
                CDSessionKey[i] = deviceSessionKey[i] ^ md5UserID[i];

            r->push_back(new InternalMessage(deviceMsg));
        }
        else
            ;//cout << "---device not online" << endl;
        clientmsg->appendContent(CDSessionKey, 16);
        uint8_t encMethod = 0x01;
        clientmsg->appendContent(encMethod);
    }
    r->push_back(new InternalMessage(clientmsg));
    return r->size();
}

// 上传场景
UploadScene::UploadScene(MicoServer  *s, ICache *c, IDelSceneUploadLog *idel, UserOperator *uop)
    : m_server(s), m_cache(c), m_useroperator(uop), m_delscenelog(idel)
{
    // 每天24点, 执行删除上传日志
    m_clocktimer.everyHour(24, [this](){
        this->m_server->asnycWork([this](){
            ::writelog("delscene log");
            this->m_delscenelog->delSceneUploadLog();
        });
    });
    // clocktimer需要一个定时器来驱动
    m_delscenepollTimer = std::make_shared<Timer>(1000 * 60 * 30); // 30 min
    m_delscenepollTimer->setTimeoutCallback([this](){
            ::writelog("delscene poll.");
            this->m_clocktimer.poll();
        });
    m_delscenepollTimer->start();
}

int UploadScene::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint16_t cmdid = reqmsg->commandID() ;
    switch (cmdid)
    {
    case CMD::CMD_HMI__UPLOAD_SCENE:
        return this->reqUpload(reqmsg, r);

    case CMD::CMD_HMI__UPLOAD_SCENE_CONFIRM:
        return this->uploadConfirm(reqmsg, r);

    default:
        return 0;
    }
}

int UploadScene::reqUpload(Message *reqmsg, std::list<InternalMessage *> *r)
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
        ::writelog("upload scene decrypt failed.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    ::writelog(InfoType, "upload scene: %lu", reqmsg->objectID());

    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    std::string scenename;
    int snlen = readString(p, plen, &scenename);
    // 场景名长度至少一个字节吧
    if (snlen <= 1)
    {
        ::writelog("upload scene scenename error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    p += snlen;
    plen -= snlen;

    uint64_t sceneid;
    uint16_t filecount;

    if (plen < sizeof(sceneid) + sizeof(filecount))
    {
        ::writelog("upload scene msg error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    sceneid = ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);

    filecount = ReadUint16(p);
    p += sizeof(filecount);
    plen -= sizeof(filecount);

    std::vector<std::pair<uint32_t, std::string> > files;
    uint32_t uploadfilesize = 0;
    for (uint16_t i = 0; i < filecount; ++i)
    {
        if (plen <= sizeof(uint32_t))
        {
            msg->appendContent(ANSC::MESSAGE_ERROR);
            ::writelog("upload scene msg error2.");
            return r->size();
        }
        uint32_t filesize = ReadUint32(p);
        p += sizeof(filesize);
        plen -= sizeof(filesize);

        std::string filename;
        int rl = readString(p, plen, &filename);
        if (rl <= 1) // 文件名至少一字节
        {
            msg->appendContent(ANSC::MESSAGE_ERROR);
            ::writelog("upload scene msg error3.");
            return r->size();
        }
        p += rl;
        plen -= rl;

        files.push_back(std::make_pair(filesize, filename));
        uploadfilesize += filesize;
    }

    uint64_t userid = reqmsg->objectID();
    // get all size and usedsize
    uint32_t allsize;
    uint32_t usedsize;

    std::string strfilenames;
    for (uint32_t i = 0; i < files.size(); ++i)
    {
        strfilenames.append(files[i].second);
        if (i != files.size() - 1)
            strfilenames.append(":");
    }

    m_useroperator->getUserDiskSpace(userid, &allsize, &usedsize);

    UploadReqData uploaddata;
    uploaddata.userid = userid;
    uploaddata.sceneid = sceneid;
    uploaddata.allsize = allsize;
    uploaddata.usedsize = usedsize;
    uploaddata.uploadfilesize = uploadfilesize;
    uploaddata.scenename = scenename;
    uploaddata.strfilenames = strfilenames;
    uploaddata.files = files;
    uploaddata.msg = msg;

    if (sceneid != 0)
    {
        this->replaceUpload(uploaddata);
        return r->size();
    }
    else
    {
        uint16_t protover = ::getProtoVersion(reqmsg->versionNumber());
        // 0x200以前:请求上传后直接把场景添加到用户的场景列表
        // 上传协议版本大于0x200后, 分两步:1.上传. 2.确认, 第1步时场景不会添加到
        // 用户的场景列表, 而是添加到一个上传记录的表, 第2步确认后再把场景添加到
        // 用户的场景列表,同时把上传记录删除
        ::writelog(InfoType, "upload protover: %x, %x", reqmsg->versionNumber(), protover);
        if (protover > 0x200)
        {
            ::writelog("upload should confirm");
            uploaddata.shouldConfirm = true;
        }
        else
        {
            ::writelog("upload should not confirm");
            uploaddata.shouldConfirm = false;
        }
        this->uploadNew(uploaddata);
        return r->size();
    }

}

int UploadScene::uploadConfirm(Message *reqmsg, std::list<InternalMessage *> *r)
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
        ::writelog("upload scene confirm decrypt failed.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    uint8_t uploadtype; // 0首次上传, 1, 覆盖上传
    uint8_t issuccess;
    if (plen != sizeof(sceneid) + sizeof(uploadtype) + sizeof(issuccess))
    {
        ::writelog("upload scene confirm messge length error.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    sceneid = ::ReadUint64(p);
    p += sizeof(sceneid);
    plen -= sizeof(sceneid);

    uploadtype = ::ReadUint8(p);
    p += 1;
    plen -= 1;

    issuccess = ::ReadUint8(p);
    p += 1;
    plen -= 1;

    if (uploadtype == 0)
    {
        // add scene to  users scene list
        if (issuccess == ANSC::SUCCESS)
        {
            ::writelog("upload scene confirm confirm success.");

            std::vector<std::pair<uint64_t, uint8_t>> target;
            target.push_back(std::make_pair(userid, SceneAuth::READ_WRITE));
            // 
            int res = m_useroperator->shareSceneToUser(userid, sceneid, target);
            if (res < 0)
            {
                ::writelog(InfoType,
                        "upload confirm share to user failed.%lu, %lu",
                        userid,
                        sceneid);
            }
            res = m_useroperator->removeUploadLog(sceneid);
            if (res < 0)
            {
                ::writelog(InfoType,
                        "upload confirm remove upload log failed.%lu, %lu",
                        userid,
                        sceneid);
            }
            // 收到确认后把场景的大小添加到空间使用表中
            uint32_t scenesize = 0;
            res = m_useroperator->getSceneSize(sceneid, &scenesize);
            if (res == -1)
            {
                ::writelog(InfoType,
                        "upload confirm get scene size failed..%lu, %lu",
                        userid,
                        sceneid);
            }
            else
            {
                res = m_useroperator->allocDisk(userid, scenesize);
                if (res == -1)
                {
                    ::writelog(InfoType,
                            "upload confirm allocdisk failed..%lu, %lu",
                            userid,
                            sceneid);
                }
            }
        }
        else
        {
            ::writelog(InfoType,"upload scene confirm confirm failed.error code: %d",(int)issuccess);
        }
    }
    else if (uploadtype == 1)
    {
        ::writelog("upload scene confirm replace type");
        // update scene version
        if (m_useroperator->incSceneVersion(sceneid) != 0)
        {
            ::writelog(ErrorType, "inc scene version failed.%lu", sceneid);
        }
    }
    else
    {
        ::writelog("upload confirm error upload type");
    }
    msg->appendContent(ANSC::SUCCESS);
    return r->size();
}

//void UploadScene::uploadNew(uint64_t userid,
//                    uint64_t sceneid,
//                    uint32_t allsize,
//                    uint32_t usedsize,
//                    uint32_t uploadfilesize,
//                    const std::string &scenename,
//                    const std::string &strfilenames,
//                    const std::vector<std::pair<uint32_t, std::string> > &files,
//                    bool shouldConfirm,
//                    Message *msg)
void UploadScene::uploadNew(const UploadReqData &d)
{
    uint32_t availableSize = d.allsize - d.usedsize;
    if (availableSize < d.uploadfilesize)
    {
        // 空间不够
        ::writelog(InfoType, "upload nospace:%lu", d.userid);
        d.msg->appendContent(ANSC::CLOUD_DISK_NOT_ENOUGH);
        return;
    }
    // ok
    uint32_t serverid;
    serverid = UploadServerInfo::genUploadServerID();
    // 
    bool shouldAddToMySceneList = !d.shouldConfirm;

    if (m_useroperator->insertScene(d.userid,
            d.scenename,
            d.uploadfilesize,
            d.strfilenames,
            serverid,
            shouldAddToMySceneList,
            const_cast<uint64_t*>(&d.sceneid)) != 0)
    {
        d.msg->appendContent(ANSC::FAILED);
        ::writelog("upload scene insert scene failed.");
        return;
    }

    setUploadUrlToMsg(d.userid, d.sceneid, serverid, d.files, d.msg);
}

//void UploadScene::replaceUpload(uint64_t userid,
//                    uint64_t sceneid,
//                    uint32_t allsize,
//                    uint32_t usedsize,
//                    uint32_t uploadfilesize,
//                    const std::string &scenename,
//                    const std::string &strfilenames,
//                    const std::vector<std::pair<uint32_t, std::string> > &files,
//                    Message *msg)
void UploadScene::replaceUpload(const UploadReqData &d)
{

    uint64_t owner;
    m_useroperator->getSceneOwner(d.sceneid, &owner);
    // 只有场景所有者才可以上传
    if (owner != d.userid)
    {
        ::writelog("upload scene noauth.");
        d.msg->appendContent(ANSC::AUTH_ERROR);
        return;
    }
    // update scene size
    // get cur scenesize
    uint32_t cursize = 0;
    if (m_useroperator->getSceneSize(d.sceneid, &cursize) != 0)
    {
        d.msg->appendContent(ANSC::FAILED);
        ::writelog("upload scene getscenesize failed.");
        return;
    }
    // 空间是否足够
    uint32_t availableSize = d.allsize - d.usedsize;
    if (d.uploadfilesize > cursize
        && ((d.uploadfilesize - cursize) > availableSize))
    {
        // 空间不够
        ::writelog(InfoType, "upload nospace:%lu", d.userid);
        d.msg->appendContent(ANSC::CLOUD_DISK_NOT_ENOUGH);
        return;
    }
    // else ok
    // 更新大小信息
    if (m_useroperator->updateSceneInfo(d.userid,
            d.scenename,
            d.sceneid,
            d.strfilenames,
            cursize,
            d.uploadfilesize) == -1)
    {
        d.msg->appendContent(ANSC::FAILED);
        ::writelog("upload scene update sceneinfo failed.");
        return;
    }
    uint32_t serverid;
    if (m_useroperator->getSceneServerid(d.sceneid, &serverid) != 0)
    {
        d.msg->appendContent(ANSC::FAILED);
        ::writelog("upload scene get sceneserver id failed.");
        return;
    }
    setUploadUrlToMsg(d.userid, d.sceneid, serverid, d.files, d.msg);
}

void UploadScene::setUploadUrlToMsg(uint64_t userid,
                uint64_t sceneid,
                uint32_t serverid, 
                const std::vector<std::pair<uint32_t, std::string> > &files,
                Message *msg)
{
    std::string url = UploadServerInfo::getServrUrl(serverid);
    std::string serverip = UploadServerInfo::getServrIP(serverid);
    if (url.empty())
        assert(false);
    // 
    std::string http("http://");
    url.insert(url.begin(), http.begin(), http.end());

    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(sceneid);
    Configure *c = Configure::getConfig();
    uint16_t urlcount = files.size();
    msg->appendContent(urlcount);
    FileUpAndDownloadRedis fr = FileUpAndDownloadRedis::getInstance(
                                            serverip,
                                            c->file_redis_port,
                                            c->redis_pwd);
    for (auto it = files.begin(); it != files.end(); ++it)
    {
        std::string urlf(url);
        urlf.append("/u/");
        // 生成一个全局唯一的标识符
        uint64_t gid = ::genGUID();
        char buf[100];
        snprintf(buf, sizeof(buf),
            "%lx",
            gid);
        std::string tag(buf);
        urlf.append(tag);
        msg->appendContent(urlf);
        if (!fr.insertUploadUrl(tag, sceneid, it->second, it->first))
        {
            ::writelog(InfoType, "upload write url to redis failed: %lu", userid);
        }
    }

    ::writelog(InfoType, "upload scene success: %lu", userid);
    return;
}

// 下载
DownloadScene::DownloadScene(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int DownloadScene::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
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
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();
    uint64_t sceneid;
    // uint64_t userid;

    if (plen < sizeof(sceneid))
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    // userid = reqmsg->ObjectID();
    sceneid = ReadUint64(p);

    // 取场景的文件列表
    std::string files;
    if (m_useroperator->getSceneFileList(sceneid, &files) != 0)
    {
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    // 取场景所在的服务器
    uint32_t serverid;
    if (m_useroperator->getSceneServerid(sceneid, &serverid) != 0)
    {
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    std::string serverurl = UploadServerInfo::getServrUrl(serverid);
    std::string serverip = UploadServerInfo::getServrIP(serverid);

    std::string http("http://");
    serverurl.insert(serverurl.begin(), http.begin(), http.end());
    std::vector<std::string> filename;
    // aaa.a:bbb.a:ccc.d
    ::splitString(files, ":", &filename);

    Configure *c = Configure::getConfig();
    FileUpAndDownloadRedis fr = FileUpAndDownloadRedis::getInstance(
                                            serverip,
                                            c->file_redis_port,
                                            c->redis_pwd);

    uint16_t urlcnt = filename.size();
    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(sceneid);
    msg->appendContent(urlcnt);
    for (auto it = filename.begin(); it != filename.end(); ++it)
    {
        if (it->empty())
            continue;

        char buf[1024];
        snprintf(buf, sizeof(buf), "%lx", ::genGUID());
        std::string uid(buf);

        fr.insertDownloadUrl(uid, sceneid, *it);
        std::string fileurl(serverurl);
        fileurl.append("/d/");
        fileurl.append(uid);
        msg->appendContent(fileurl);
        msg->appendContent(*it); // filename
    }

    return r->size();
}

// ---
TransferScene::TransferScene(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int TransferScene::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
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

    char *p = reqmsg->content();
    uint32_t plen = reqmsg->contentLen();

    uint64_t userid = reqmsg->objectID();
    uint64_t sceneid;
    uint64_t targetuserid;
    if (plen < sizeof(sceneid) + sizeof(targetuserid))
    {
        ::writelog("transfer message error meeeage.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    sceneid = ReadUint64(p);
    p += sizeof(sceneid);

    targetuserid = ReadUint64(p);
    p += sizeof(targetuserid);

    // 操作者是场景的所有者
    if (!m_useroperator->hasShareAuth(userid, sceneid))
    {
        ::writelog(InfoType, "transfer scene noauth of user %lu", userid);
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }
    // 接收者用户要存在且对场景有权限
    bool hasauth;
    if (m_useroperator->isUserHasAccessRightOfScene(sceneid, targetuserid, &hasauth) < 0)
    {
        ::writelog(InfoType, "transfer scene check auth failed. %lu", targetuserid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    if (!hasauth)
    {
        ::writelog(InfoType, "transfer scene target user has no auth. %lu", targetuserid);
        msg->appendContent(ANSC::AUTH_ERROR);
        return r->size();
    }

    // 取场景大小
    uint32_t scenesize;
    if (m_useroperator->getSceneSize(sceneid, &scenesize) != 0)
    {
        ::writelog(InfoType, "transfer scene get scene size failed.%lu", sceneid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }
    // 取目标用户空间大小
    uint32_t allsize, usedsize;
    if (m_useroperator->getUserDiskSpace(targetuserid, &allsize, &usedsize) != 0)
    {
        ::writelog(InfoType, "transfer scene get user diskspace failed.%lu", targetuserid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    if ((allsize - usedsize) < scenesize)
    {
        ::writelog(InfoType,
            "transfer scene target use has no disk.%lu:%lu->%lu",
            sceneid, userid, targetuserid);
        msg->appendContent(ANSC::CLOUD_DISK_NOT_ENOUGH);
        return r->size();
    }

    if (m_useroperator->freeDisk(userid, scenesize) < 0)
    {
        ::writelog(InfoType,
            "transfer scene free disk failed.%lu:%lu->%lu",
            sceneid, userid, targetuserid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    if (m_useroperator->allocDisk(targetuserid, scenesize) != 0)
    {
        ::writelog(InfoType,
            "transfer scene alloc disk failed.%lu:%lu->%lu",
            sceneid, userid, targetuserid);
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    if (m_useroperator->setSceneOwnerID(sceneid, targetuserid) < 0)
    {
        ::writelog("transfer scene set owner id failed.");
        msg->appendContent(ANSC::FAILED);
        return r->size();
    }

    std::vector<uint64_t>  users; users.push_back(targetuserid);
    if (m_useroperator->setShareSceneAuth(targetuserid, sceneid, SceneAuth::READ_WRITE, users) < 0)
    {
        ::writelog("transfer scene modify auth failed.");
    }

    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(sceneid);
    msg->appendContent(targetuserid);

    // 生成一个通知, 告诉目标用户你有一个新场景
    genSceneTransferNotify(userid, targetuserid, sceneid, r);

    // -用户的设备列表变化, 发送此消息
    // 此消息的处理器UserDeviceListChanged会更新这个用户的场景连接设备
    // 把没有权限的设备删除
    //Message *updatesceneconnector = Message::createClientMessage();
    //updatesceneconnector->SetCommandID(CMD::USER_DEVICE_LIST_CHANGED);
    //updatesceneconnector->SetObjectID(targetuserid);
    //InternalMessage *sceneimsg = new InternalMessage(
    //    updatesceneconnector,
    //    InternalMessage::Internal,
    //    InternalMessage::Unused);
    //MainQueue::postCommand(new NewMessageCommand(sceneimsg));
    // 场景被移交后,那么原来场景里面连接的设备,可能会有一些变得不可用
    // 所以要把这些不可用的设备从场景的连接里面删除掉
    // 这里可以进行一些优化, 因为己经知道场景id了
    SceneConnectDevicesOperator sceneop(m_useroperator);
    sceneop.findInvalidDevice(targetuserid);

    return r->size();
}

void TransferScene::genSceneTransferNotify(uint64_t userid,
        uint64_t targetuserid,
        uint64_t sceneid,
        std::list<InternalMessage *> *r)
{
    // 
    std::string account;
    std::string scenename;
    if (m_useroperator->getUserAccount(userid, &account) != 0)
        return;
    if (m_useroperator->getSceneName(sceneid, &scenename) != 0)
        return;
    std::vector<char> notifybuf;
    ::WriteUint64(&notifybuf, userid);
    ::WriteString(&notifybuf, account);
    ::WriteUint64(&notifybuf, sceneid);
    ::WriteString(&notifybuf, scenename);

    Notification notify;
    notify.Init(targetuserid, // 通知接收者ID
        ANSC::SUCCESS,
        CMD::CMD_HMI__TRANSFER_SYSADMIN, // 通知号/通知类型
        0, // 序列号, 在别的地方生成
        notifybuf.size(), // 内容长度
        &notifybuf[0]);// 内容

    Message *msg = new Message;
    msg->setCommandID(CMD::CMD_NEW_NOTIFY);
    msg->setObjectID(targetuserid); // 通知接收者的ID
    msg->appendContent(&notify);

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setEncrypt(false);
    imsg->setType(InternalMessage::ToNotify);

    r->push_back(imsg);
}

// --
GetClientDeviceSessionkey::GetClientDeviceSessionkey(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetClientDeviceSessionkey::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
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

    if (reqmsg->contentLen() < sizeof(uint64_t))
    {
        ::writelog("get client device session error message.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    uint64_t deviceID = ReadUint64(reqmsg->content());

    char devSessionKey[16] = {0};
    char CDSessionKey[16] = {0};
    uint8_t answerCode = 0;

    uint64_t userID = reqmsg->objectID();
    // TODO FIXME 还要从redis取, 如果没有取到
    shared_ptr<DeviceData> deviceData =
        static_pointer_cast<DeviceData>(m_cache->getData(deviceID));

    if (!deviceData)
    {
        deviceData = static_pointer_cast<DeviceData>(RedisCache::instance()->getData(deviceID));
    }

    if (deviceData)
    {
        std::vector<char> session = deviceData->GetCurSessionKey();
        assert(session.size() == 16); // session 长度为16

        memcpy(devSessionKey, &(session[0]), session.size());

        MD5 md5;
        char md5UserID[16] = {0};
        md5.input((const char*) &userID, sizeof(userID)); // usrid 生成md5
        md5.output(md5UserID);
        for(size_t i = 0; i != 16; i++) { // 然后用旧的sessionkey与userid md5 运算
            CDSessionKey[i] = devSessionKey[i] ^ md5UserID[i];
        }
        answerCode = ANSC::SUCCESS;
    }
    else
        answerCode = ANSC::DEVICE_OFFLINE;

    msg->appendContent(&answerCode, sizeof(answerCode));
    msg->appendContent(&deviceID, sizeof(deviceID));
    msg->appendContent(CDSessionKey, 16);

    return r->size();
}

CloseRelay::CloseRelay(ICache *c)
    : m_cache(c)
{
}

int CloseRelay::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
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

    char *p = reqmsg->content();
    uint16_t plen = reqmsg->contentLen();
    uint16_t count = 0;
    if (plen < sizeof(count))
    {
        ::writelog("close msg len error, error meeeage.");
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }
    count = ::ReadUint16(p);
    p += sizeof(count);
    plen -= sizeof(count);

    uint64_t devid = 0;
    uint64_t userid = reqmsg->objectID();
    if (plen != count * sizeof(devid))
    {
        ::writelog("close msg len.2 error, error meeeage.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }
    msg->appendContent(ANSC::SUCCESS);

    // 发消息到relay关闭转发通道
    Message *torelay = ::Message::createClientMessage();
    InternalMessage *torelayimsg = new InternalMessage(torelay);
    torelayimsg->setEncrypt(false);
    torelayimsg->setType(InternalMessage::ToRelay);
    r->push_back(torelayimsg);
    //.
    RelayMessage rmsg;
    rmsg.setCommandCode(Relay::CLOSE_CHANNEL);
    rmsg.setUserid(userid);
    // read id
    for (;count > 0; --count)
    {
        devid = ::ReadUint64(p);
        p += sizeof(devid);
        plen -= sizeof(devid);
        RelayDevice rd;
        rd.devicdid = devid;
        rmsg.appendRelayDevice(rd);
    }
    torelay->appendContent(&rmsg);

    return r->size();
}


SceneFileChanged::SceneFileChanged(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int SceneFileChanged::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    Message *msg = new Message;
    msg->CopyHeader(reqmsg);
    msg->setDestID(reqmsg->objectID());
    InternalMessage *imsg = new InternalMessage(msg);

    imsg->setEncrypt(true);
    // 添加到输出
    r->push_back(imsg);

    if(::decryptMsg(reqmsg, m_cache) != 0)
    {
        ::writelog("cmd scene changed: decrypt failed, error meeeage.");
        // 解密失败
        msg->appendContent(&ANSC::MESSAGE_ERROR, Message::SIZE_ANSWER_CODE);
        return r->size();
    }

    char *p = reqmsg->content();
    int plen = reqmsg->contentLen();

    std::string scenename;
    int readlen = ::readString(p, plen, &scenename);
    if (readlen <= 0)
    {
        ::writelog("cmd scene changed, read scene name error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    p += readlen;
    plen -= readlen;

    uint64_t sceneid = 0;
    if (plen != sizeof(sceneid))
    {
        ::writelog("cmd scene changed, sceneid len error.");
        msg->appendContent(ANSC::MESSAGE_ERROR);
        return r->size();
    }

    sceneid = ::ReadUint64(p);

    // get shared users ...
    genNotifySceneFileChanged(reqmsg->objectID(),
        sceneid,
        scenename,
        r);

    msg->appendContent(ANSC::SUCCESS);
    msg->appendContent(sceneid);
    return r->size();
}

void SceneFileChanged::genNotifySceneFileChanged(uint64_t ownerid,
        uint64_t sceneid,
        const std::string &scenename,
        std::list<InternalMessage *> *r)
{
    std::string owneraccount;
    m_useroperator->getUserAccount(ownerid, &owneraccount);

    std::vector<SharedSceneUserInfo> vs;
    vs.reserve(100);

    if (m_useroperator->getSceneSharedUses(ownerid, sceneid,&vs) == -1)
        return;


    std::vector<char> notifydata;
    notifydata.reserve(100);

    ::WriteUint64(&notifydata, ownerid);
    ::WriteString(&notifydata, owneraccount);
    ::WriteUint64(&notifydata, sceneid);
    ::WriteString(&notifydata, scenename);

    Notification notify;
    notify.Init(0, // 通知接收者ID
        ANSC::SUCCESS,
        CMD::CMD_SCENE_FILE_CHANGED, // 通知号/通知类型
        0, // 序列号, 在别的地方生成
        notifydata.size(), // 内容长度
        &notifydata[0]);// 内容

    for (auto it = vs.begin(); it != vs.end(); ++it)
    {
        if (it->userid == ownerid)
            continue;
        notify.SetObjectID(it->userid);

        Message *msg = new Message;
        msg->setCommandID(CMD::CMD_NEW_NOTIFY);
        msg->setObjectID(it->userid); // 通知接收者的ID
        msg->appendContent(&notify);

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        imsg->setType(InternalMessage::ToNotify);
        r->push_back(imsg);
    }
}
