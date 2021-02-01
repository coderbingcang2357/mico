#include "claimmanager.h"

ClaimManager::ClaimManager()
{
}

bool ClaimManager::insertClaimRequest(const std::shared_ptr<ClaimDevice> &d)
{
    MutexGuard g(&mutex);
    std::pair<ClaimIteraror, bool> res = claimreq.insert(d);
    return res.second;
}

bool ClaimManager::removeClaimRequest(uint64_t userid, uint64_t deviceid)
{
    std::shared_ptr<ClaimDevice>  ccd(new ClaimDevice(userid, deviceid, ""));
    MutexGuard g(&mutex);
    auto it = claimreq.find(ccd);
    if (it != claimreq.end())
    {
        claimreq.erase(it);
        return true;
    }
    else
        return false;
}

bool ClaimManager::isExist(ClaimDevice*d)
{
    if (get(d->userID(), d->deviceID()))
        return true;
    else
        return false;
}

std::shared_ptr<ClaimDevice> ClaimManager::get(uint64_t userid, uint64_t deviceid)
{
    std::shared_ptr<ClaimDevice>  ccd(new ClaimDevice(userid, deviceid, ""));
    MutexGuard g(&mutex);
    auto it = claimreq.find(ccd);
    if (it != claimreq.end())
    {
        return *it;
    }
    else
        return std::shared_ptr<ClaimDevice>();
}

