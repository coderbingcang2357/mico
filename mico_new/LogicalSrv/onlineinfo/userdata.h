#ifndef USERDATA__H
#define USERDATA__H
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include <string>
#include <vector>
//#include "thread/mutexwrap.h"
#include <string.h>
#include "iclient.h"

class Timer;
class UserData : public IClient
{
public:
    enum OnlineStatus {UserOffLine, UserOnLine};

    UserData();
    ~UserData();

    uint64_t id();

    time_t lastHeartBeatTime();
    void setLastHeartBeatTime(time_t t);

    time_t loginTime();
    void setLoginTime(time_t t);

    //uint8_t onlineStatus();
    //void setOnlineStatus(uint8_t s);

    void setSockAddr(const sockaddrwrap & addr) { m_sockaddr = addr;}
    sockaddrwrap &sockAddr() { return m_sockaddr; }

    void setVersionNumber(uint16_t vernum) { m_versionNumber = vernum;}
    uint16_t versionNumber() {return m_versionNumber; }

    void setClientVersion(uint16_t v) {m_clientVersion = v;}
    uint16_t clientVersion() {return m_clientVersion;}

    // .
    std::vector<char> sessionKey();
    void setSessionKey(const std::vector<char> &sess);
    uint32_t loginServerID() {return m_loginserverid; }
    std::string name() { return this->account; }

    std::string toStringJson();
    bool fromStringJson(const std::string &strjson);


    bool toByteArray(char *buf, int *len);
    bool fromByteArray(char *buf, int *len);

    void setLoginServer(uint32_t sid) { m_loginserverid = sid; }

    int connectionServerId() override;
    uint64_t connectionId() override;

    uint64_t userid;
    uint16_t lastSerialNumber;
    std::string account; // name

    void setConnectionId(const uint64_t &connectionId);

    void setConnectionServerId(int connectionServerId);

private:
    uint16_t m_versionNumber;

    std::vector<char> m_sessionKey;
    sockaddrwrap m_sockaddr;
    //uint8_t m_onlineStatus;
    time_t m_logintime;
    time_t m_lastHeartBeatTime;
    uint32_t m_loginserverid;
    uint16_t m_clientVersion;
    uint64_t m_connectionId;
    int m_connectionServerId;
};


// 
class UserDataByteArray
{
public:
    int             classVersion;
    uint64_t        userid;
    uint16_t        lastSerialNumber;
    char            account[64]; // name

    char            sessionKey[16];
    uint16_t        versionNumber;
    sockaddrwrap    sockaddr;

    //uint8_t         onlineStatus;
    time_t          logintime;
    time_t          lastHeartBeatTime;
    uint32_t        loginserverid;
    uint16_t        clientVersion;
};
#endif
