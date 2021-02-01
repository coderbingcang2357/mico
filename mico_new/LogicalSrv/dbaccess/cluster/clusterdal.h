#ifndef CLUSTERDAL__H__
#define CLUSTERDAL__H__
#include <stdint.h>
#include <string>

// data access layer
class ClusterDal
{
public:
    ClusterDal();

    int getDeviceClusterIDByAccount(const std::string &clusteracc, uint64_t *);
    int insertDevice(const std::string &name,
            uint64_t firmClusterID,
            uint64_t macAddr,
            uint8_t encryptionMethod,
            uint8_t keyGenerationMethod,
            const std::string &strLoginKey,
            const std::string &deviceClusterAccount,
            uint8_t type,
            uint8_t transferMethod,
            uint64_t previousID,
            uint64_t *deviceID);

    int maxUserCount();
    int maxDeviceCount();
    int userCount(uint64_t cluid);
    int deviceCount(uint64_t cluid);
    bool canAddUser(uint64_t cluid);
    bool canAddDevice(uint64_t cluid, int count);

private:
    int m_maxuser;
    int m_maxDevice;

    friend class TestClusterDal;
};
#endif
