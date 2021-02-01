#include "icache.h"
#include "devicedata.h"
#include <iostream>
#include "deviceOfflineCommand.h"
#include "protocoldef/protocol.h"
#include "Message/Message.h"
//#include "../Message/Message.h"
//#include "../Message/MsgWriter.h"
//#include "../ShareMem/ShareMem.h"
#include "msgqueue/ipcmsgqueue.h"
#include "rediscache.h"

void DeviceOfflineCommand::execute(void *p)
{
    //std::cout << "deviceoffline " << m_dev->deviceID() << std::endl;
    ICache *ol = static_cast<ICache *>(p);
    std::shared_ptr<DeviceData> dev = static_pointer_cast<DeviceData>(
            ol->getData(m_devid));
    ol->removeObject(m_devid);

    // remove from redis
    RedisCache::instance()->removeObject(m_devid);

    // 设备下线, 生成通知消息, 通设备群中所有的用户设备下线了
    uint16_t cmdID = CMD::INCMD_DEVICE_OFFLINE;
    Message msg;
    msg.SetSockAddr(dev->sockAddr());//后续可能有用
    msg.SetCommandID(cmdID);
    msg.SetObjectID(m_devid);
    //msg.SetContentLen(0);

    //MsgWriter msgWriter(ShareMem::IpcAddrOutputToIntConnSrv());
    //msgWriter.Write(&msg);
    //MsgWriterPosixMsgQueue msgWriter(::msgqueueLogicalToInternalSrv());
    //msgWriter.Write(&msg);
}

