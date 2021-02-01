#ifndef CLAIM_MANAGER__H
#define CLAIM_MANAGER__H

#include <set>
#include "iclaimmanager.h"
#include "claimdevice.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"

class ClaimManager : public IClaimManager
{
public:
    ClaimManager();
    ~ClaimManager(){}
    bool insertClaimRequest(const std::shared_ptr<ClaimDevice> &d);
    bool removeClaimRequest(uint64_t userid, uint64_t deviceid);
    bool isExist(ClaimDevice *d);
    std::shared_ptr<ClaimDevice> get(uint64_t useid, uint64_t deviceid);

private:
    typedef std::set<std::shared_ptr<ClaimDevice>, ClaimDeviceComparable>::iterator ClaimIteraror;
    MutexWrap mutex;
    std::set<std::shared_ptr<ClaimDevice>, ClaimDeviceComparable> claimreq;
};
#endif
