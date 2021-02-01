#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <map>
#include <sys/select.h>
#include <stdint.h>
#include <set>
#include <netinet/in.h>
#include <memory>
#include <sys/time.h>
#include <string.h>
#include "userdevicepair.h"
#include "util/sock2string.h"

struct ChannelInfo
{
    ChannelInfo()
    {
        userfd = -1;
        devfd = -1;
        ::memset(&userSockAddr, 0, sizeof(userSockAddr));
        ::memset(&deviceSockAddr, 0, sizeof(deviceSockAddr));
        ::gettimeofday(&lastCommuniteTime, 0);
    }
    uint64_t channelid; // 每次打开通道时分配一个唯一的id
    int userfd;
    int devfd;
    uint16_t userport;
    uint16_t deviceport;
    uint64_t userID;
    uint64_t deviceID;
    uint64_t userSessionRandomNumber;
    uint64_t devSessionRandomNumber;
    sockaddrwrap userSockAddr;
    sockaddrwrap deviceSockAddr;
    timeval lastCommuniteTime;
};

class ChannelManager
{
public:
    ChannelManager();
    ~ChannelManager();

    void insert(const std::shared_ptr<ChannelInfo> &ch);
    void openChannel(const std::shared_ptr<ChannelInfo> &ch);
    void closeChannel(const UserDevPair &ud);
    std::shared_ptr<ChannelInfo> findChannel(const UserDevPair &ud);
    bool updateAddress(uint64_t randomnumber, const sockaddrwrap &addr);
    void poll();

private:
    std::map<UserDevPair, std::shared_ptr<ChannelInfo>> m_channels;
    // find channel by random number
    std::map<uint64_t, std::shared_ptr<ChannelInfo>> m_rndChannels;
};


#endif
