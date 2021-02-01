#ifndef I_CLAIM_DEVICE_H
#define I_CLAIM_DEVICE_H

#include <stdint.h>
#include <memory>
class ClaimDevice;
class IClaimManager
{
public:
    virtual ~IClaimManager() {}

    virtual bool insertClaimRequest(const std::shared_ptr<ClaimDevice> &d) = 0;
    virtual bool removeClaimRequest(uint64_t userid, uint64_t deviceid) = 0;
    virtual bool isExist(ClaimDevice *d) = 0;
    virtual std::shared_ptr<ClaimDevice> get(uint64_t userid, uint64_t deviceid) = 0;
};
#endif
