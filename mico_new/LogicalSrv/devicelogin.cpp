#include <memory>
#include "onlineinfo/isonline.h"
#include "devicelogin.h"
#include "useroperator.h"
#include "sessionkeygen.h"
#include "onlineinfo/devicedata.h"
#include "protocoldef/protocol.h"
#include "Crypt/tea.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "onlineinfo/heartbeattimer.h"
#include "onlineinfo/rediscache.h"
#include "onlineinfo/userdata.h"
#include "Message/Message.h"
#include "config/configreader.h"
#include "messageencrypt.h"
#include "Message/Notification.h"
#include "thread/mutexguard.h"
#include "server.h"

DeviceLoginProcessor::DeviceLoginProcessor(MicoServer *server, UserOperator *uop)
    : m_devoperator(uop), m_server(server)
{
}

int DeviceLoginProcessor::processMesage(Message *reqmsg, std::list<InternalMessage *> *r)
{
    switch (reqmsg->commandID())
    {
    case CMD::DCMD__LOGIN1:
    {
        DeviceLoginRequestData d;
        d.deviceid = reqmsg->objectID();
        DeviceLoginResponseData rep = this->loginReq(d);

        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setObjectID(reqmsg->objectID()); 
        msg->setDestID(0);
        msg->appendContent(&rep);

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        //OutputMsg(imsg, ToOutside);
        r->push_back(imsg);
    }
    break;

    case CMD::DCMD__LOGIN2:
    {
        DeviceLoginVerfyData d;
        d.deviceid = reqmsg->objectID();
        char *p = reqmsg->content();
        int len = reqmsg->contentLen();
        d.connectionid = reqmsg->connectionId();
        d.connectionServerid = reqmsg->connectionServerid();
        d.sockaddr = reqmsg->sockerAddress();
        d.randomnumberAfterEncrypt = std::move(std::vector<char>(p, p + len));
        DeviceLoginResult result = this->loginVerfy(d);

        Message *msg = new Message;
        msg->CopyHeader(reqmsg);
        msg->setDestID(0);
        if (result.answccode == ANSC::SUCCESS)
        {
            EncryptByteArray encba(&result, &result.loginkey);
            msg->appendContent(&encba);

            result.devicedata->setSockAddr(reqmsg->sockerAddress());
            saveDataToCacheAndRedis(result.devicedata);
            notifyMemberDeviceOnline(result.clusterID,
                                     result.devicedata->deviceID(),
                                     r);
        }
        else
        {
            msg->appendContent(&result);
        }

        InternalMessage *imsg = new InternalMessage(msg);
        imsg->setEncrypt(false);
        //OutputMsg(imsg, ToOutside);
        r->push_back(imsg);
    }
    break;
    }

    return r->size();
}

DeviceLoginResponseData DeviceLoginProcessor::loginReq(
    const DeviceLoginRequestData &d)
{
    DeviceLoginResponseData res;
    res.deviceid = d.deviceid;
    res.answccode = ANSC::SUCCESS;
    res.randomnumber = ::getRandomNumber();

    this->insertRandomNumber(res.deviceid, res.randomnumber);
    return res;
}

DeviceLoginResult DeviceLoginProcessor::loginVerfy(
    const DeviceLoginVerfyData &d)
{
    DeviceLoginResult r;
    DeviceInfo *devinfo = m_devoperator->getDeviceInfo(d.deviceid);
    std::unique_ptr<DeviceInfo> _del(devinfo);
    if (devinfo == 0)
    {
        // can not find device in sql server
        r.answccode = ANSC::FAILED;
        r.devicedata = 0;
        return r;
    }
    char bufnumber[1024];
    const char *p = &d.randomnumberAfterEncrypt[0];
    int dlen = d.randomnumberAfterEncrypt.size();
    int len = tea_dec(const_cast<char*>(p), dlen,
                bufnumber,
                &(devinfo->loginkey[0]), devinfo->loginkey.size());
    if (len < 0)
    {
        r.answccode = ANSC::FAILED;
        r.devicedata = 0;
        ::writelog(InfoType, "device login dec failed: %lu", d.deviceid);
        return r;
    }

    uint64_t randnumber = ReadUint64(bufnumber);
    uint64_t reqrandomnumber;
    if (!getRandomNumber(d.deviceid, &reqrandomnumber))
    {
        // 没有找到randomnumber
        r.answccode = ANSC::FAILED;
        r.devicedata = 0;
        ::writelog(InfoType,
            "device login failed, can not find randomnumber: %lu", d.deviceid);
        return r;
    }

    if (reqrandomnumber != randnumber)
    {
        // failed
        r.answccode = ANSC::FAILED;
        r.devicedata = 0;
        ::writelog(InfoType,
            "device login failed, randomnumber error: %lu", d.deviceid);
        return r;
    }
    // login success
    // gen sessionkey
    //char sessionkey[16];
    std::list<uint64_t> ids{devinfo->id};
    m_devoperator->setDeviceStatus(ids, DeviceStatu::Active);

    r.answccode = ANSC::SUCCESS;
    DeviceData *devdata = new DeviceData;
    r.devicedata = std::shared_ptr<DeviceData>(devdata);
    r.clusterID = devinfo->clusterID;
    r.loginkey = devinfo->loginkey;

    devdata->setDeviceID(devinfo->id);
    //devdata->clusterID = devinfo->clusterID;
    devdata->SetName(devinfo->name);
    devdata->setMacAddr(devinfo->macAddr);

    std::vector<char> sessionkey;
    ::genSessionKey(&sessionkey);
    devdata->setSessionKey(sessionkey);
    devdata->setNewSessionKey(sessionkey);
    devdata->setSockAddr(d.sockaddr);
    devdata->setLoginTime(time(0));
            //
    Configure *c = Configure::getConfig();
    devdata->setLoginServer(c->serverid);
    devdata->setConnectionId(d.connectionid);
    devdata->setConnectionServerId(d.connectionServerid);

    return r;
}

void DeviceLoginProcessor::insertRandomNumber(
    uint64_t deviceid, uint64_t randomnumber)
{

    MutexGuard g(&m_mutex);

    m_randomnumber[deviceid] = randomnumber;
}

bool DeviceLoginProcessor::getRandomNumber(
    uint64_t deviceid, uint64_t *outnumber)
{
    MutexGuard g(&m_mutex);

    auto it = m_randomnumber.find(deviceid);
    if (it != m_randomnumber.end())
    {
        *outnumber = it->second;
        m_randomnumber.erase(it);
        return true;
    }
    else
    {
        return false;
    }
}

void DeviceLoginProcessor::saveDataToCacheAndRedis(const std::shared_ptr<DeviceData> &devicedata)
{
    //char* pos = m_requestMsg->Content();

    //char sessionKey[16] = {0};
    uint64_t deviceID = devicedata->deviceID();
    //uint64_t clusterID = devicedata->clusterID; // //ReadUint64(pos += 16); // 设备所属的群号???

    writelog(InfoType, "LogicalSrv, Device Online: %lu", deviceID);

    std::shared_ptr<DeviceData> deviceData(devicedata);

    m_server->onlineCache()->insertObject(deviceID, deviceData);
    // create connection
    // m_server->createUdpConnection(deviceData->sockAddr(), deviceID);

    // 注册超时定时器
    ::registerDeviceTimer(deviceID);

    // 缓存到redis, 当设备掉线时就会从redis删除
    RedisCache::instance()->insertObject(deviceID, deviceData);

    // TODO fix me 设备会有什么通知?
    //GetMsgFromNotifSrv(deviceID);
    // 更新最后登录时间
    time_t curt = time(0);
    m_server->asnycWork([deviceID, curt, this](){
        this->m_devoperator->updateDeviceLastLoginTime(deviceID, curt);
        });
}

void DeviceLoginProcessor::notifyMemberDeviceOnline(
    uint64_t clusterid,
    uint64_t devid,
    std::list<InternalMessage *> *r)
{
    // 取设备所在群的所有人
    std::list<uint64_t> vUserMemberID;
    // DevClusDBOperator dcOperator(m_sql);
    //DeviceOperator devoperator;
    if(m_devoperator->getUserIDInDeviceCluster(clusterid, &vUserMemberID) < 0)
    {
        return;
    }

    std::vector<char> buf;
    ::WriteUint64(&buf, clusterid);
    ::WriteUint64(&buf, devid);
    ::WriteUint8(&buf, ONLINE);
    Notification notify;
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::NTP_DEV__DEVICE_ONLINE_STATUS_CHANGE,
        0,
        buf.size(),
        &buf[0]);

    // 设备属于某个群, 把设备上线的通知发给这个群中所有的人 看起来是这个意思??? 没看懂...
    for (auto it = vUserMemberID.begin(); it != vUserMemberID.end(); ++it)
    {
        uint64_t userID = *it;
        //pos += sizeof(userID);
        //cout << "--user ID: " << userID << endl;

        std::shared_ptr<UserData> userData
            = dynamic_pointer_cast<UserData>(
                                    m_server->onlineCache()->getData(userID));

        if (!userData)
        {
            userData = dynamic_pointer_cast<UserData>(
                            RedisCache::instance()->getData(userID));
        }
        if (!userData)
            continue;

        //if(userData->onlineStatus() == Cache::OS_Online) {
        if (userData)
        {
            notify.SetObjectID(userID);
            Message* notifMsg = new Message();
            notifMsg->setObjectID(userID); // 接收者id
            notifMsg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
            notifMsg->appendContent(&notify);

            InternalMessage *imsg = new InternalMessage(notifMsg);
            imsg->setType(InternalMessage::ToNotify);
            imsg->setEncrypt(false);
            r->push_back(imsg);
        }
    }
}

void DeviceLoginResponseData::toByteArray(std::vector<char> *o)
{
    // 1字节应答码, 8 字节randomnumber
    o->push_back(answccode);
    char *p = reinterpret_cast<char *>(&(this->randomnumber));
    o->insert(o->end(), p, p + sizeof(randomnumber));
}

void DeviceLoginResult::toByteArray(std::vector<char> *o)
{
    // 1字节结果, 16字节sessionkey
    o->push_back(this->answccode);

    std::vector<char> sessionkey(16, 0);
    if (this->devicedata)
    {
        sessionkey = this->devicedata->GetSessionKey();
    }
    assert(sessionkey.size() == 16);
    o->insert(o->end(), &sessionkey[0], &sessionkey[0] + sessionkey.size());
}

// 设备心跳
DeviceHeartBeat::DeviceHeartBeat(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int DeviceHeartBeat::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if (reqmsg->commandID() == CMD::DCMD__LONG_HEART_BEAT)
    {
        return longheartbeat(reqmsg, r);
    }
    else if (reqmsg->commandID() == CMD::DCMD__SHORT_HEART_BEAT)
    {
        if (::decryptMsg(reqmsg, m_cache) < 0)
        {
            return 0;
        }
        else
        {
            return shortheartbeat(reqmsg, r);
        }
    }
    else
    {
        return 0;
    }
}

int DeviceHeartBeat::longheartbeat(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint64_t deviceID = reqmsg->objectID();

    shared_ptr<DeviceData> deviceData 
        = dynamic_pointer_cast<DeviceData>(m_cache->getData(deviceID));

    if(!deviceData)
    {
        ::writelog(InfoType, "can not find cache, %lu\n", deviceID);
        return 0;
    }

    deviceData->setLastHeartBeatTime(time(NULL));

    char sessionKey[16] = {0};
    char newSessionKey[16] = {0};
    std::vector<char> session = deviceData->GetSessionKey();
    assert(session.size() == 16);
    std::vector<char> vecnewsession = deviceData->getNewSessionKey(); 
    assert(vecnewsession.size() == 16);

    memcpy(sessionKey, &(session[0]), 16);
    memcpy(newSessionKey, &(vecnewsession[0]), 16);
    // 
    uint8_t keyUpdateStatus = deviceData->GetKeyUpdateStatus(); // ???什么时候要更新?
    // 处理通信密钥(sessionKey)动态修改, 三种状态 need update, updating updated
    // 每过一段时间修改一次sessionKey, 当要修改时状态变为 needupdate, 
    // !![这个功能目前没有启用, 所以密钥是不会变的]
    bool useNewKey = false;
    bool sendNewKey = false;
    switch(keyUpdateStatus){
        case Cache::KUS_NeedUpdate:
            if(reqmsg->Decrypt(sessionKey) < 0) {
                //std::cout << "---need update, sessionKey cannot decrypt" << endl;
                return 0;
            }
            deviceData->SetKeyUpdateStatus(Cache::KUS_Updating);
            sendNewKey = true;
            break;
        case Cache::KUS_Updating:
            if(reqmsg->Decrypt(newSessionKey) >= 0) {
                //std::cout << "---updating, use new sessionKey" << endl;
                deviceData->SetKeyUpdateStatus(Cache::KUS_Updated);
                deviceData->SetLastKeyUpdateTime(time(NULL));
                useNewKey = true;
            }
            else if(reqmsg->Decrypt(sessionKey) >= 0){
                //std::cout << "---updating, use old sessionKey" << endl;
                sendNewKey = true;
            }
            else {
                //std::cout << "---updating, all sessionKey cannot decrypt" << endl;
                return 0;
            }
            break;
        case Cache::KUS_Updated:
            if(reqmsg->Decrypt(newSessionKey) < 0) {
                //std::cout << "---updated, sessionKey cannot decrypt" << endl;
                return 0;
            }
            useNewKey = true;
            break;
        default:
            return 0;
            break;
    }

    // ??? 这个数据的格式在哪?
    char* pos = reqmsg->content();
    uint8_t prefix = ReadUint8(pos);
    uint8_t opcode = ReadUint8(pos += sizeof(prefix));
    uint8_t clusterAccountLen = ReadUint8(pos += sizeof(opcode));
    uint8_t deviceNameLen = ReadUint8(pos += sizeof(clusterAccountLen));
    uint8_t reservedLen= ReadUint8(pos += sizeof(deviceNameLen));
    string clusterAccount(pos += sizeof(reservedLen), clusterAccountLen);
    string deviceName(pos += clusterAccountLen, deviceNameLen);
    uint64_t macAddr = ReadMacAddr(pos += deviceNameLen + reservedLen);
    (void)macAddr;

    opcode = 0xA1;  // 0xA1 ???

    Configure *conf = Configure::getConfig();
    uint16_t serverPort = htons(conf->extconn_srv_port);
    uint32_t serverIP = inet_addr(conf->extconn_srv_address.c_str());

    Message* responseMsg = new Message;
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->objectID());
    responseMsg->setSockerAddress(reqmsg->sockerAddress());
    responseMsg->appendContent(&prefix, sizeof(prefix));
    responseMsg->appendContent(&opcode, sizeof(opcode));
    responseMsg->appendContent(&serverIP, sizeof(serverIP));
    responseMsg->appendContent(&serverPort, sizeof(serverPort));
    responseMsg->appendContent(&deviceID, sizeof(deviceID));

    uint8_t keyLen = 0;
    if(sendNewKey) {
        keyLen = 16;
        responseMsg->appendContent(&keyLen, sizeof(keyLen));
        responseMsg->appendContent(newSessionKey, 16);
    }
    else
        responseMsg->appendContent(&keyLen, sizeof(keyLen));

    if(useNewKey)
        responseMsg->Encrypt(newSessionKey);
    else
        responseMsg->Encrypt(sessionKey);
    InternalMessage *imsg = new InternalMessage(responseMsg);
    imsg->setEncrypt(false);
    r->push_back(imsg);

    deviceData->setSockAddr(reqmsg->sockerAddress());

    if(deviceName != deviceData->GetName()){
        //std::cout << "Name changed." << std::endl;
        ::writelog(InfoType,
            "Name changed, old: %s, new: %s",
            deviceData->GetName().c_str(),
            deviceName.c_str());
        deviceData->SetName(deviceName);
        deviceNameChanged(deviceData, deviceName, r);
    }

    // 更新数据
    m_cache->insertObject(deviceID, deviceData);

    return r->size();
}

int DeviceHeartBeat::shortheartbeat(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint64_t deviceID = reqmsg->objectID();
    //char deviceSessionKey[16] = {0};

    //CacheManager cacheManager;
    //DeviceData deviceData = cacheManager.GetDeviceData(deviceID);
    shared_ptr<DeviceData> deviceData 
        = dynamic_pointer_cast<DeviceData>(m_cache->getData(deviceID));
    //if(deviceData.Empty())
    if (!deviceData)
    {
        ::writelog(InfoType,
                "short heart beat, can not find device, %ld",
                deviceID);
        return 0;
    }

    deviceData->setLastHeartBeatTime(time(NULL));

    /*std::vector<char> cursession = deviceData->GetCurSessionKey();
    assert(cursession.size() != 0);
    //memcpy(deviceSessionKey, deviceData.GetCurSessionKey(), 16);
    memcpy(deviceSessionKey, &(cursession[0]), 16);
    if(reqmsg->Decrypt(deviceSessionKey) < 0) {
        ::writelog("---can not decrypt short heart beat");
        std::vector<char> s = deviceData->GetSessionKey();
        if (s.size() > 0 )
        {
            if (reqmsg->Decrypt(&(s[0])) < 0)
            {
                return 0; // failed
            }
            else
            {
                // ok
                //std::cout << "sessionkey, decrypt ok short heart beat" << endl;
                memcpy(deviceSessionKey, &(s[0]), 16);
            }
        }
        else
        {
            // session key error
            return 0;
        }
    }*/ // else ok

    char* pos = reqmsg->content();
    uint8_t prefix = ReadUint8(pos);
    uint8_t opcode = ReadUint8(pos += sizeof(prefix));
    uint8_t reservedLen = ReadUint8(pos += sizeof(opcode));
    uint64_t macAddr = ReadMacAddr(pos += (sizeof(reservedLen) + reservedLen));

    (void)macAddr;
    //cout << "---device short heartbeat: id: " << deviceID << endl;
    // cout << "---mac addr: " << hex << macAddr << dec << endl;

    opcode = 0xA1;

//#ifdef _DEBUG_VERSION
    //uint16_t serverPort = htons(8888);
    //uint32_t serverIP = inet_addr("203.195.201.120");
    Configure *conf = Configure::getConfig();
    uint16_t serverPort = htons(conf->extconn_srv_port);
    uint32_t serverIP = inet_addr(conf->extconn_srv_address.c_str());
    //uint16_t serverPort = htons(ExtConn_Srv_Port );
    //uint32_t serverIP = inet_addr(ExtConn_Srv_Address);
//#else
    //uint16_t serverPort = htons(8888);
    //uint32_t serverIP = inet_addr("203.195.237.53");
//#endif

    Message* responseMsg = new Message;
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->objectID());
    responseMsg->setSockerAddress(reqmsg->sockerAddress());
    responseMsg->appendContent(&prefix, sizeof(prefix));
    responseMsg->appendContent(&opcode, sizeof(opcode));
    responseMsg->appendContent(&serverIP, sizeof(serverIP));
    responseMsg->appendContent(&serverPort, sizeof(serverPort));
    responseMsg->appendContent(&deviceID, sizeof(deviceID));
    // responseMsg->Encrypt(deviceSessionKey);
    InternalMessage *imsg = new InternalMessage(responseMsg);
    imsg->setEncrypt(true);
    r->push_back(imsg);

    //deviceData.SetLastHeartBeatTime(time(NULL));
    //deviceData.SetSockAddr(m_requestMsg->SockAddr());
    deviceData->setSockAddr(reqmsg->sockerAddress());

    // 更新数据
    m_cache->insertObject(deviceID, deviceData);

    return r->size();
}

// 更新数据库
int DeviceHeartBeat::deviceNameChanged(
    const std::shared_ptr<DeviceData> &device,
    const std::string &name, std::list<InternalMessage *> *r)
{
    if (m_useroperator->modifyDeviceName(device->deviceID(), name) != 0)
        return 0;

    return notifyMemberDeviceNameChanged(device->deviceID(), name, r);
}

// 生成一个通知发到客户端
int DeviceHeartBeat::notifyMemberDeviceNameChanged(
    uint64_t deviceID,
    const std::string &deviceName,
    std::list<InternalMessage *> *r)
{
    // 1. 取得设备所在的群
    DeviceInfo *device;
    if ((device = m_useroperator->getDeviceInfo(deviceID)) == 0)
    {
        return 0;
    }
    std::unique_ptr<DeviceInfo> del__(device);

    std::list<uint64_t> idlist;
    // 2. 取设备所在群的所有人
    if (m_useroperator->getUserIDInDeviceCluster(device->clusterID, &idlist) < 0)
    {
        return 0;
    }

    ::writelog("Name changed notif.");

    std::vector<char> buf;
    ::WriteUint64(&buf, device->clusterID);
    ::WriteUint64(&buf, deviceID);
    ::WriteString(&buf, deviceName);
    Notification notify;
    notify.Init(0,
        ANSC::SUCCESS,
        CMD::NTP_CMD_DEVICE_NAME_CHANGED,
        0,
        buf.size(),
        &buf[0]);

    for (auto it = idlist.begin(); it != idlist.end(); ++it)
    {
        uint64_t userID = *it;
        shared_ptr<UserData> userData
            = dynamic_pointer_cast<UserData>(m_cache->getData(userID));
        if (!userData)
        {
            userData = dynamic_pointer_cast<UserData>(RedisCache::instance()->getData(userID));
        }
        if (!userData)
            continue;

        if (userData)
        {
            ::writelog("Send Name changed notif.");
            notify.SetObjectID(userID);

            Message *notifMsg = Message::createClientMessage();

            notifMsg->setObjectID(0); 
            notifMsg->setDestID(userID); // 接收者id

            notifMsg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY); 

            notifMsg->appendContent(&notify);

            InternalMessage *imsg = new InternalMessage(notifMsg);
            imsg->setType(InternalMessage::ToNotify);
            imsg->setEncrypt(false);
            r->push_back(imsg);
        }
    }

    return r->size();
}

GetMyDeviceOnlineStatus::GetMyDeviceOnlineStatus(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{
}

int GetMyDeviceOnlineStatus::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    if(decryptMsg(reqmsg, m_cache) != 0)
    {
        Message* responseMsg = Message::createClientMessage();
        responseMsg->CopyHeader(reqmsg);
        responseMsg->setDestID(reqmsg->objectID());
        uint8_t answerCode = ANSC::FAILED;
        responseMsg->appendContent(&answerCode, sizeof(answerCode));
        r->push_back(new InternalMessage(responseMsg));
        return r->size();
    }

    char* pos = reqmsg->content();
    uint16_t deviceCount = ReadUint16(pos);
    pos += sizeof(deviceCount);

    Message* responseMsg = Message::createClientMessage();
    responseMsg->CopyHeader(reqmsg);
    responseMsg->setDestID(reqmsg->objectID());
    uint8_t answerCode = ANSC::SUCCESS;
    responseMsg->appendContent(&answerCode, sizeof(answerCode));
    responseMsg->appendContent(&deviceCount, sizeof(deviceCount));
    // 取在线状态
    //CacheManager cacheManager;
    IsOnline isonline(m_cache, RedisCache::instance());
    for(size_t i = 0; i != deviceCount; i++) {
        uint64_t deviceID = ReadUint64(pos);
        pos += sizeof(deviceID);

        uint8_t onlineStatus = isonline.isOnline(deviceID) ? ONLINE : OFFLINE;
        //cout << "---device ID: " << deviceID << ", online: " << ((onlineStatus == 1) ? 'Y' : 'N') << endl;

        responseMsg->appendContent(&deviceID, sizeof(deviceID));
        responseMsg->appendContent(&onlineStatus, sizeof(onlineStatus));;
    }
    // EncryptMsg(responseMsg);
    InternalMessage *imsg = new InternalMessage(responseMsg);
    imsg->setEncrypt(true);
    r->push_back(imsg);
    return r->size();
}

// ---
DeviceOffline::DeviceOffline(ICache *c, UserOperator *uop)
    : m_cache(c), m_useroperator(uop)
{

}

int DeviceOffline::processMesage(
    Message *reqmsg, std::list<InternalMessage *> *r)
{
    uint64_t devid;
    //char *p = reqmsg->Content();
    devid = reqmsg->objectID();

    // 取设备所在的群
    uint64_t clusterID;
    DeviceInfo *d;
    if((d = m_useroperator->getDeviceInfo(devid)) == 0)
    {
        return 0;
    }
    clusterID = d->clusterID;
    delete d;

    std::vector<UserMemberInfo> vUserMemberID;
    vUserMemberID.reserve(100);
    // 取群里面的用户
    if(0 != m_useroperator->getUserMemberInfoList(clusterID, &vUserMemberID))
    {
        return 0;
    }

    if(!vUserMemberID.empty())
    {
        std::vector<char> buf;
        ::WriteUint64(&buf, clusterID);
        ::WriteUint64(&buf, devid);
        ::WriteUint8(&buf, OFFLINE); // 在线状态

        Notification notify;
        notify.Init(0,
            ANSC::SUCCESS,
            CMD::NTP_DEV__DEVICE_ONLINE_STATUS_CHANGE,
            0,
            buf.size(),
            &buf[0]);

        for (auto iter = vUserMemberID.begin();
            iter != vUserMemberID.end();
            ++iter)
        {
            uint64_t userID = iter->id;

            shared_ptr<UserData> userData
                = dynamic_pointer_cast<UserData>(m_cache->getData(userID));

            if(!userData)
            {
                userData = dynamic_pointer_cast<UserData>(RedisCache::instance()->getData(userID));
            }
            if (!userData)
                continue;

            //if(userData.GetOnlineStatus() == Cache::OS_Online) {
            if (userData)
            {
                Message* notifMsg = new Message();
                notifMsg->CopyHeader(reqmsg);
                notifMsg->setObjectID(userID);
                notifMsg->setDestID(userID); // 通知接收者
                notifMsg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
                notify.SetObjectID(userID); // 通知接收者
                notifMsg->appendContent(&notify);

                InternalMessage *imsg = new InternalMessage(notifMsg);
                imsg->setType(InternalMessage::ToNotify);
                imsg->setEncrypt(false);
                r->push_back(imsg);
            }

        }
    }

    return r->size();
}

