#ifndef ONLINEINFO___H
#define ONLINEINFO___H

#include <stdint.h>
#include <netinet/in.h>
#include <map>
#include <list>
#include "icache.h"
#include "util/sharedptr.h"
#include "hiredis/hiredis.h"
#include "thread/mutexwrap.h"

class UserData;
class DeviceData;
class Timer;

class OnLineInfo : public ICache
{
public:
    OnLineInfo();
    ~OnLineInfo();
    bool init();
    bool objectExist(uint64_t objid);
    void insertObject(uint64_t objid, const shared_ptr<UserData> &data);
    void insertObject(uint64_t objid, const shared_ptr<DeviceData> &data);
    void removeObject(uint64_t objid);
    std::shared_ptr<IClient> getData(uint64_t objid);
    std::list<std::shared_ptr<IClient>> getAll();

private:
    MutexWrap lock;
    std::map<uint64_t, shared_ptr<IClient> > m_onlineinfo;
};

#endif

