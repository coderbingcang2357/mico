#ifndef DEVICE_DATA__H
#define DEVICE_DATA__H
#include <stdint.h>
#include <netinet/in.h>
#include <string>
#include <assert.h>
#include <vector>
#include "config/shmconfig.h" // for cache::keyupdated
#include "iclient.h"

class DeviceData : public IClient
{
public:
    DeviceData(uint64_t devid = 0);
    ~DeviceData();

    uint64_t id();

    uint64_t deviceID()
    {
        return m_devid;
    }

    void setDeviceID(uint64_t id) 
    {
        m_devid = id;
    }

    sockaddrwrap &sockAddr() override
    {
        return m_address;
    }

    std::string name() { return this->m_name; }

    uint32_t loginServerID()
    {
        return m_loginserverID;
    }
    void setSockAddr(const sockaddrwrap &sockAddr)
    {
        m_address = sockAddr;
    }

    uint16_t versionNumber()
    {
        return m_versionNumber;
    }

    void setVersionNumber(uint16_t versionNumber)
    {
        m_versionNumber = versionNumber;
    }

    time_t loginTime()
    {
        return m_logintime;
    }

    void setLoginTime(time_t t)
    {
        m_logintime = t;
    }

    time_t lastHeartBeatTime()
    {
        return m_lastHeartBeatTime;
    }

    void setLastHeartBeatTime(time_t heartBeatTime)
    {
        m_lastHeartBeatTime = heartBeatTime;
    }

    // virtual 
    std::vector<char> sessionKey()
    {
        return m_sessionKey;
    }
    // virtual 
//    std::string loginServerIpAndPort()
//    {
//        return m_loginserveripport;
//    }

    void setLoginServer(uint32_t sid)
    {
        m_loginserverID = sid;
    }

    std::vector<char> GetSessionKey()
    {
        return m_sessionKey;
    }

    void setSessionKey(const std::vector<char> &sessionKey)
    {
        assert(sessionKey.size() == 16);
        m_sessionKey = sessionKey;
    }

    std::vector<char> getNewSessionKey()
    {
        return m_newsessionKey;
    }

    void setNewSessionKey(const std::vector<char> &sessionKey)
    {
        assert(sessionKey.size() == 16);
        m_newsessionKey = sessionKey;
    }

    std::vector<char> GetCurSessionKey()
    {
        if (m_keyupdateStatus == Cache::KUS_Updated)
            return this->m_newsessionKey;
        else
            return this->m_sessionKey;
    }

    uint16_t lastSerialNumber()
    {
        return m_lastSerialNumber;
    }

    void setLastSerialNumber(uint16_t serialNumber)
    {
        m_lastSerialNumber = serialNumber;
    }

    time_t getLastKeyUpdateTime()
    {
        return m_lastKeyUpdateTime;
    }

    void SetLastKeyUpdateTime(time_t keyUpdateTime)
    {
        m_lastKeyUpdateTime = keyUpdateTime;
    }

    uint8_t GetKeyUpdateStatus()
    {
        return m_keyupdateStatus;
    }

    void SetKeyUpdateStatus(uint8_t keyUpdateStatus)
    {
        m_keyupdateStatus = keyUpdateStatus;
    }

    std::vector<char> getMacAddr()
    {
        return m_macAddr;
    }

    void setMacAddr(const std::vector<char> &mac)
    {
        assert(mac.size() == 6);
        m_macAddr = mac;
    }

    std::string GetName()
    {
        return m_name;
    }

    void SetName(const std::string &name)
    {
        m_name = name;
    }

    int connectionServerId() override;
    uint64_t connectionId() override;
    void setConnectionId(const uint64_t &connectionId);
    void setConnectionServerId(int connectionServerId);

    std::string toStringJson();
    bool fromStringJson(const std::string &strjson);

    bool toByteArray(char *buf, int *len);
    bool fromByteArray(char *buf, int *len);

private:
    uint64_t                m_devid;
    sockaddrwrap            m_address;
    uint16_t                m_versionNumber;
    time_t                  m_logintime;
    time_t                  m_lastHeartBeatTime;

    std::vector<char>       m_sessionKey; // 16 bytes
    std::vector<char>       m_newsessionKey;
    uint16_t                m_lastSerialNumber;
    time_t                  m_lastKeyUpdateTime;

    uint8_t                 m_keyupdateStatus; // updateing updated needupdate
    std::vector<char>       m_macAddr; // 6 bytes
    std::string             m_name;          // 32 bytes???
    uint32_t                m_loginserverID;
    uint64_t                m_connectionId = 0;
    int                     m_connectionServerId = 0;

    friend class DeviceDataByteArray;
};

class DeviceDataByteArray
{
public:
    DeviceDataByteArray(){}

    int                 classVersion;

    uint64_t            devid;
    sockaddrwrap        address;
    uint16_t            versionNumber;
    time_t              logintime;
    time_t              lastHeartBeatTime;

    time_t              lastKeyUpdateTime;
    char                sessionKey[16]; // 16 bytes
    char                newsessionKey[16];
    uint16_t            lastSerialNumber;

    uint8_t             keyupdateStatus; // updateing updated needupdate
    char                macAddrhex[12]; // 12 bytes
    char                name[64];          // 32 bytes???
    uint32_t            loginserverID;
};
#endif
