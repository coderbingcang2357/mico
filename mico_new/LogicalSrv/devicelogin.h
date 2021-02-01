#ifndef DEVICE_LOGIN_H
#define DEVICE_LOGIN_H

#include <stdint.h>
#include <map>
#include <memory>
#include <netinet/in.h>

#include "Message/itobytearray.h"
#include "imessageprocessor.h"
#include "onlineinfo/icache.h"
#include "thread/mutexwrap.h"

class DeviceData;
class UserOperator;
class MicoServer;

class DeviceLoginRequestData
{
public:
    uint64_t deviceid;
};

class DeviceLoginResponseData : public IToByteArray
{
public:
    void toByteArray(std::vector<char> *o);
    uint8_t  answccode;
    uint64_t deviceid;
    uint64_t randomnumber;
};

class DeviceLoginVerfyData
{
public:
    uint64_t deviceid;
    sockaddrwrap sockaddr;
    std::vector<char> randomnumberAfterEncrypt;
    int connectionServerid;
    uint64_t connectionid;
};

class DeviceLoginResult : public IToByteArray
{
public:
    void toByteArray(std::vector<char> *o);

    uint8_t answccode;
    std::shared_ptr<DeviceData> devicedata;
    std::vector<char>       loginkey;
    uint64_t clusterID;
};

class DeviceLoginProcessor : public IMessageProcessor
{
public:
    DeviceLoginProcessor(MicoServer *server, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    DeviceLoginResponseData loginReq(const DeviceLoginRequestData &d);
    DeviceLoginResult loginVerfy(const DeviceLoginVerfyData &d);
    void saveDataToCacheAndRedis(const std::shared_ptr<DeviceData> &devicedata);
    void notifyMemberDeviceOnline(uint64_t clusterid, uint64_t devid,
        std::list<InternalMessage *> *r);

    void insertRandomNumber(uint64_t deviceid, uint64_t randomnumber);
    bool getRandomNumber(uint64_t deviceid, uint64_t *outnumber);

    MutexWrap m_mutex;
    std::map<uint64_t, uint64_t> m_randomnumber;

    UserOperator *m_devoperator;
    MicoServer  *m_server;
};

class DeviceHeartBeat : public IMessageProcessor
{
public:
    DeviceHeartBeat(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    int longheartbeat(Message *, std::list<InternalMessage *> *r);
    int shortheartbeat(Message *, std::list<InternalMessage *> *r);

    int deviceNameChanged(const std::shared_ptr<DeviceData> &device,
        const std::string &name,
        std::list<InternalMessage *> *r);
    int notifyMemberDeviceNameChanged(
        uint64_t devid,
        const std::string &deviceName,
        std::list<InternalMessage *> *r);

    ICache *m_cache;
    UserOperator *m_useroperator;
};

class GetMyDeviceOnlineStatus : public IMessageProcessor
{
public:
    GetMyDeviceOnlineStatus(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

class DeviceOffline : public IMessageProcessor
{
public:
    DeviceOffline(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};
#endif
