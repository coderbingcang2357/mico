#ifndef REDIS_CACHE__H
#define REDIS_CACHE__H

#include <stdint.h>
#include "icache.h"
#include "util/sharedptr.h"
#include "hiredis/hiredis.h"
#include <list>

class UserData;
class DeviceData;
class RedisConnectionPool;

class RedisCache : public ICache
{
public:
    static RedisCache *instance();

    RedisCache();
    ~RedisCache();
    bool objectExist(uint64_t objid);
    void insertObject(uint64_t objid, const shared_ptr<UserData> &data);
    void insertObject(uint64_t objid, const shared_ptr<DeviceData> &data);
    void removeObject(uint64_t objid);
    shared_ptr<IClient> getData(uint64_t objid);
    std::list<std::shared_ptr<IClient>> getAll();

private:
    void insert(uint64_t objid,
                const std::string &json);

    RedisConnectionPool *pool;
};

#endif

