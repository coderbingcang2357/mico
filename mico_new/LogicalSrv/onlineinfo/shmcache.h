#ifndef SHM_CACHE_H
#define SHM_CACHE_H

#include "icache.h"
#include "util/sharedptr.h"
#include "msgqueue/memoryhash.h"
#include "msgqueue/shmwrap.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"

class Shmcache : public ICache
{
public:
    Shmcache();
    ~Shmcache();
    bool init();
    bool objectExist(uint64_t objid);
    void insertObject(uint64_t objid, const shared_ptr<UserData> &data);
    void insertObject(uint64_t objid, const shared_ptr<DeviceData> &data);
    void removeObject(uint64_t objid);
    shared_ptr<IClient> getData(uint64_t objid);
    std::list<std::shared_ptr<IClient>> getAll();

private:
    MutexWrap mutex;
    MemoryHash m_memhash;
    ShmWrap m_shmwrap;
};
#endif
