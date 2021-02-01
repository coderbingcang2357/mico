#include <time.h>
#include <iostream>
#include "icache.h"
#include "devicedata.h"
#include "devicetimeoutcommand.h"
#include "../sessionkeygen.h"
#include "protocoldef/protocol.h"
#include "Message/Message.h"
//#include "../Message/Message.h"
//#include "../Message/MsgWriter.h"
//#include "../ShareMem/ShareMem.h"
#include "msgqueue/ipcmsgqueue.h"
#include "rediscache.h"
#include "heartbeattimer.h"
#include "util/logwriter.h"
#include "mainthreadmsgqueue.h"
#include "../newmessagecommand.h"
#include "../server.h"
#include "../useroperator.h"

static int HeartBeatTimeout = 30;
static int KeyUpdateInterval = 60;

DeviceTimeoutCommand::DeviceTimeoutCommand(uint64_t id)
    : ICommand(DeviceTimeout), m_devid(id)
{
}

void DeviceTimeoutCommand::execute(MicoServer *server)
{
    ICache *ol = server->onlineCache();

    shared_ptr<DeviceData> dev = static_pointer_cast<DeviceData>(ol->getData(m_devid));

    if (!dev)
    {
        ::writelog(InfoType, "can not find device in cache %lu", m_devid);
        return;
    }

    time_t now = time(NULL);
    time_t old = dev->lastHeartBeatTime();
    time_t lastKeyTime = dev->getLastKeyUpdateTime();
    double elaptime = difftime(now, old);
    double elapKeyUpdateTime = difftime(now, lastKeyTime);

    // 检查心跳时间, 判断设备是否下线, 设备下线发一个消息到时主线程的消息
    // 队列, 把设备从缓存(OnlineInfo)中删除
    if (elaptime >= 3 * HeartBeatTimeout)
    {
        //std::cout << "device offline" << dev->deviceID() << std::endl;
        ::writelog(InfoType, "device offline %lu", dev->deviceID());

        //DeviceOfflineCommand *doc = new DeviceOfflineCommand(this
        //                                            , ICommand::DeviceOffLine);
        // send a messge to cache manage, that i'm offline
        //printf("device offline, elaptime: %lf, cmd: %lx\n", elaptime,  uint64_t(doc));
        //getMainThreadMsgQueue()->put(doc);

        // remove udp connection
        //server->removeUdpConnection(dev->sockAddr(), dev->deviceID());
        // remove from local cache
        ol->removeObject(dev->deviceID());

        // 删除超时定时器
        ::unregisterTimer(m_devid);

        // remove from redis
        auto devdatafromredis = RedisCache::instance()->getData(dev->deviceID());
        // serverid是一样的说明这个记录是就是这台服务器插入的, 所以可以直接删除
        // 否则就是别的服务器插入的记录, 则不可以在本服务器上删除
        if (devdatafromredis && devdatafromredis->loginServerID() == dev->loginServerID())
        {
            RedisCache::instance()->removeObject(dev->deviceID());
        }

        // 设备下线, 生成通知消息, 通设备群中所有的用户设备下线了
        uint16_t cmdID = CMD::INCMD_DEVICE_OFFLINE;
        Message *msg = new Message;
        msg->setSockerAddress(dev->sockAddr());//后续可能有用
        msg->setCommandID(cmdID);
        msg->setObjectID(dev->deviceID());
        //msg.SetContentLen(0);
        // TODO fixme
        InternalMessage *imsg
            = new InternalMessage(msg,
                InternalMessage::Internal,
                InternalMessage::Unused);

        MainQueue::postCommand(new NewMessageCommand(imsg));

        // 更新在线时长
        time_t onlinetime = time(0) - dev->loginTime();
        server->asnycWork([this, server, onlinetime](){
            server->userOperator()->updateDeviceOnlineTime(this->m_devid,
                onlinetime);
            });
    }
    // 检查是否到了更新sessionKey时间, 更新sessionKey
    else if (elapKeyUpdateTime > 3 * KeyUpdateInterval)
    {
        // 这个功能不稳定先不要
        return ;

        dev->setSessionKey(dev->getNewSessionKey());
        char buf[16];
        genSessionKey(buf, sizeof(buf));
        dev->setNewSessionKey(std::vector<char>(buf, buf + sizeof(buf)));

        dev->SetKeyUpdateStatus(Cache::KUS_NeedUpdate);

        dev->SetLastKeyUpdateTime(now);
        ::writelog(InfoType, "key update: %lu", dev->deviceID());
        ol->insertObject(m_devid, dev);
    }
    else
    {
        ;
    }
}

