#ifndef REPLAYPORTS__H__
#define REPLAYPORTS__H__
#include <stdint.h>
#include <map>
#include <list>
#include <set>
#include "./userdevicepair.h"

class PortPair
{
public:
    operator bool () const
    {
        return userport > 0 && devport > 0 && channelid > 0;
    }
    // 一个portpair代表一个新分配的转发通道, 以道次分配时生成一个channelid
    uint64_t channelid = 0;
    int userfd = -1;
    int devicefd = -1;
    uint16_t userport = 0;
    uint16_t devport = 0;
};

class RelayPorts
{
    class PortSocket
    {
    public:
        operator bool () const
        {
            return socketfd >= 0 && port > 0;
        }

        int socketfd = -1;
        uint16_t port = 0;
        std::set<uint64_t> deviceids;// 这个正在使用这个port的device
    };

public:
    RelayPorts();
    void add(int sockfd, uint16_t port);
    PortPair allocate(uint64_t userid, uint64_t deviceid);
    void release(uint64_t userid, uint64_t deviceid, uint64_t chid);
    void insert(uint64_t userid, uint64_t deviceid, const PortPair &pp);

    static void test();
private:
    uint16_t findValidDevicePort(uint64_t devid);

    std::map<uint16_t, PortSocket> m_ports;
    std::map<UserDevPair, PortPair> m_channels;
    uint64_t m_channelid = 1;
};

#endif
