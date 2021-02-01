#ifndef ICACHE___H
#define ICACHE___H

#include <stdint.h>
#include <list>
#include "util/sharedptr.h" 
#include "iclient.h"

class UserData;
class DeviceData;

class ICache
{
public:
    virtual ~ICache() {}
    virtual bool objectExist(uint64_t objid) = 0;
    virtual void insertObject(uint64_t objid, const shared_ptr<UserData> &data) = 0;
    virtual void insertObject(uint64_t objid, const shared_ptr<DeviceData> &data) = 0;
    virtual void removeObject(uint64_t objid) = 0;
    virtual shared_ptr<IClient> getData(uint64_t objid) = 0;
    virtual std::list<std::shared_ptr<IClient>> getAll() = 0;
};
#endif
