#include <assert.h>
#include "onlineinfo.h"
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "userdata.h"
#include "devicedata.h"
#include "config/configreader.h"
#include "thread/mutexguard.h"
#include "rediscache.h"
#include "heartbeattimer.h"

OnLineInfo::OnLineInfo()
{
}

OnLineInfo::~OnLineInfo()
{
}

bool OnLineInfo::init()
{
    uint32_t serverid = Configure::getConfig()->serverid;
    auto clients = RedisCache::instance()->getAll();
    for (auto &c : clients)
    {
        if (c->loginServerID() != serverid)
            continue;
        uint64_t id = c->id();
        IdType idtype = ::GetIDType(id);
        if (idtype == IT_User)
        {
            auto userdata = static_pointer_cast<UserData>(c);
            // 因为lastHeartbeattime是没有实时更新到redis中去的
            // 所以这里要重新设置
            userdata->setLastHeartBeatTime(time(0));
            this->insertObject(c->id(), userdata);
            ::registerUserTimer(id);
        }
        else if (idtype == IT_Device)
        {
            // 因为lastHeartbeattime是没有实时更新到redis中去的
            // 所以这里要重新设置
            auto devdata = static_pointer_cast<DeviceData>(c);
            devdata->setLastHeartBeatTime(time(0));
            this->insertObject(c->id(), devdata);
            ::registerDeviceTimer(id);
        }
        else
        {
            ::writelog(WarningType, "error id type.%lu", id);
            assert(false);
        }
    }

    return true;
}

bool OnLineInfo::objectExist(uint64_t objid)
{
    MutexGuard g(&lock);
    bool ishere = m_onlineinfo.find(objid) != m_onlineinfo.end();
    return ishere;
}

void OnLineInfo::insertObject(uint64_t objid, const shared_ptr<UserData> &data)
{
    MutexGuard g(&lock);
    m_onlineinfo[objid] = data;
}

void OnLineInfo::insertObject(uint64_t objid, const shared_ptr<DeviceData> &data)
{
    MutexGuard g(&lock);
    m_onlineinfo[objid] = data;
}

void OnLineInfo::removeObject(uint64_t objid)
{
    MutexGuard g(&lock);
    m_onlineinfo.erase(objid);
}

shared_ptr<IClient> OnLineInfo::getData(uint64_t objid)
{
    MutexGuard g(&lock);
    //std::map<uint64_t, void *>::iterator it = m_onlineinfo.find(objid);
    auto it = m_onlineinfo.find(objid);
    if (it != m_onlineinfo.end())
    {
        IdType idtype = ::GetIDType(it->second->id());
        switch (idtype)
        {
        case IT_User:
            return std::shared_ptr<UserData>(new UserData(*static_pointer_cast<UserData>(it->second)));

        case IT_Device:
            return std::shared_ptr<DeviceData>(new DeviceData(*static_pointer_cast<DeviceData>(it->second)));

        default:
            return shared_ptr<IClient>();
        }
    }
    else
    {
        return shared_ptr<IClient>();
    }
}

std::list<std::shared_ptr<IClient>> OnLineInfo::getAll()
{
    MutexGuard g(&lock);
    std::list<std::shared_ptr<IClient>> ret;
    for (auto &v : m_onlineinfo)
    {
        ret.push_back(v.second);
    }
    return ret;
}
