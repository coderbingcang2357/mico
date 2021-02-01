#include "shmcache.h"
#include "config/configreader.h"
#include "userdata.h"
#include "devicedata.h"
#include "heartbeattimer.h"
#include "util/logwriter.h"
#include "protocoldef/protocol.h"

Shmcache::Shmcache()
{

}

Shmcache::~Shmcache()
{

}

bool Shmcache::init()
{
    Configure *cfg = Configure::getConfig();
    (void)cfg;
    int elesize = sizeof(UserDataByteArray) > sizeof(DeviceDataByteArray) ?
                    sizeof(UserDataByteArray) :
                    sizeof(DeviceDataByteArray);
    int shmsize = cfg->shmOnlineCacheSize;
    // 确保是1024的整数倍
    if (shmsize % 1024 != 0)
    {
        shmsize = shmsize + shmsize % 1024;
    }

    if (!m_shmwrap.create("/shm_online_info_cache", shmsize))
    {
        printf("shm create failed.\n");
        return false;
    }
    m_memhash.init(m_shmwrap.address(), m_shmwrap.length(), elesize);
    ::writelog(InfoType, "hashinfo: slot:%d, max:%d, size:%d\n",
                m_memhash.slotSize(), m_memhash.maxElement(), m_memhash.size());

    // 从共享内存中加载登录信息
    auto it = m_memhash.iterator();
    // int cnt = 0;
    for (;it.hasNext();)
    {
        it.next();

        uint64_t key = (*it)->first;
        if (GetIdType(key) == IT_Device)
        {
            ::registerDeviceTimer(key);
        }
        else if (GetIdType(key) == IT_User)
        {
            //UserDataByteArray *b = (UserDataByteArray *)((*it)->second);
            //if (b->onlineStatus == UserData::UserOnLine)
            //{
                ::registerUserTimer(key);
                //printf("%d, find key: %lu\n", ++cnt, key);
            //}
        }
        else
            assert(false);
    }
    return true;
}

bool Shmcache::objectExist(uint64_t objid)
{
    MutexGuard g(&mutex);

    char *d = (char *)m_memhash.get(objid);
    return d != 0;
}

void Shmcache::insertObject(uint64_t objid, const shared_ptr<UserData> &data)
{
    UserDataByteArray ba;
    int len = sizeof(UserDataByteArray);
    data->toByteArray((char *)&ba, &len);

    MutexGuard g(&mutex);

    bool r = m_memhash.set(objid, (char *)&ba, len);
    //assert(r);
    if (!r)
    {
        ::writelog(InfoType, "shm hash full, insertfailed: %lu", objid);
        // should i release memory????
    }
}

void Shmcache::insertObject(uint64_t objid, const shared_ptr<DeviceData> &data)
{
    DeviceDataByteArray ba;
    int len = sizeof(DeviceDataByteArray);
    data->toByteArray((char *)&ba, &len);

    MutexGuard g(&mutex);

    bool r = m_memhash.set(objid, (char *)&ba, len);
    assert(r);
    if (!r)
    {
        ::writelog(InfoType, "shm hash full, insertfailed: %lu", objid);
        // should i release memory????
    }
}

void Shmcache::removeObject(uint64_t objid)
{
    MutexGuard g(&mutex);

    bool r = m_memhash.remove(objid);
    // assert(r);
    if (!r)
    {
        ::writelog(InfoType, "memhash remove failed: %lu", objid);
    }
}

shared_ptr<IClient> Shmcache::getData(uint64_t objid)
{
    MutexGuard g(&mutex);

    char *d = (char *)m_memhash.get(objid);
    if (d == 0)
        return shared_ptr<IClient>();

    if (GetIdType(objid) == IT_User)
    {
        UserData *u = new UserData;
        int len = sizeof(UserDataByteArray);
        u->fromByteArray(d, &len);
        return shared_ptr<UserData>(u);
    }
    else if (GetIdType(objid) == IT_Device)
    {
        DeviceData *dev = new DeviceData;
        int len = sizeof(DeviceDataByteArray);
        dev->fromByteArray(d, &len);
        return shared_ptr<DeviceData>(dev);
    }
    else
    {
        assert(false);
        return shared_ptr<IClient>();
    }
}

std::list<std::shared_ptr<IClient>> Shmcache::getAll()
{
    assert(false);
    return std::list<std::shared_ptr<IClient>>();
}
