#include <assert.h>
#include "relayports.h"
#include "util/logwriter.h"
#include "util/micoassert.h"

RelayPorts::RelayPorts()
{
}

void RelayPorts::add(int sockfd, uint16_t port)
{
    PortSocket ps;
    ps.port = port;
    ps.socketfd = sockfd;
    m_ports.insert(std::make_pair(port, ps));
}

// 要保证:
// 1. PortSocket.deivceids中的设备id是唯一的
// 重复打开返回相同的port
PortPair RelayPorts::allocate(uint64_t userid, uint64_t deviceid)
{
    PortPair pp;
    // 生成channelid
    pp.channelid = m_channelid++;
    auto chs = m_channels.find(UserDevPair(userid, deviceid));
    // already
    if (chs != m_channels.end())
    {
        pp.userport = chs->second.userport;
        pp.userfd = chs->second.userfd;
        pp.devport = chs->second.devport;
        pp.devicefd = chs->second.devicefd;
        chs->second.channelid = pp.channelid;
    }
    // find new
    else
    {
        // 用户的port永远用第一个??还是随机给呢???
        auto firstport = m_ports.begin();
        // 用户port永远给第一个
        pp.userport = firstport->second.port;
        pp.userfd = firstport->second.socketfd;
        pp.devport = findValidDevicePort(deviceid);
        if (pp.devport == 0)
        {
            // error
            ::writelog(ErrorType, "no valid port.");
        }
        else
        {
            // ok, add devid to port's deviceids list
            m_ports[pp.devport].deviceids.insert(deviceid);
            pp.devicefd = m_ports[pp.devport].socketfd;
            // and add a channel info to m_channels
            m_channels.insert(std::make_pair(UserDevPair(userid, deviceid), pp));
            ::writelog(InfoType,
                "allocate port for %lu,%lu, user port: %u, fd: %d, dev port: %u, fd:%d, chid: %lu",
                userid, deviceid,
                pp.userport, 
                pp.userfd,
                pp.devport,
                pp.devicefd,
                pp.channelid);
        }
    }
    return pp;
}

void RelayPorts::release(uint64_t userid, uint64_t deviceid, uint64_t channelid)
{
    // 查看channels是否存在
    auto it = m_channels.find(UserDevPair(userid, deviceid));
    if (it != m_channels.end())
    {
        // ok find it
        // 取设备所使用的port
        PortPair pp = it->second;
        // channelid必需匹配
        if (pp.channelid == channelid)
        {
            auto devportit = m_ports.find(pp.devport);
            assert(devportit != m_ports.end());
            ::writelog(InfoType,
                "release port:userid: %lu, devid: %lu, portuser:%u, portdev:%u, chid:%lu",
                userid,
                deviceid,
                it->second.userport,
                it->second.devport,
                it->second.channelid);
            // remove id from deviceids
            devportit->second.deviceids.erase(deviceid);
            m_channels.erase(it);
        }
        else
        {
            ::writelog(ErrorType, "release port error chid not match:%lu, %lu, chid1: %lu, chid2: %lu",
                    userid,
                    deviceid,
                    pp.channelid,
                    channelid);
        }
    }
    else
    {
        //
        ::writelog(ErrorType, "release port error:%lu, %lu, chid: %lu", userid, deviceid, channelid);
    }
}

// 查找一个port, 它的deviceids列表中没有devid
uint16_t RelayPorts::findValidDevicePort(uint64_t devid)
{
    for (auto it = m_ports.begin(); it != m_ports.end(); ++it)
    {
        if (it->second.deviceids.find(devid) == it->second.deviceids.end())
        {
            // ok
            return it->second.port;
        }
    }
    // no valid port
    return 0;
}

void RelayPorts::insert(uint64_t userid, uint64_t deviceid, const PortPair &pp)
{
    // devport 要存在
    assert(userid > 0 && deviceid > 0);
    m_channels.insert(std::make_pair(UserDevPair(userid, deviceid), pp));
    m_ports[pp.devport].deviceids.insert(deviceid);
}

void RelayPorts::test()
{
    ::writelog("start test RelayPorts");
    try
    {
        RelayPorts rp;
        for (int i = 0; i < 2; ++i)
        {
            rp.add(i + 1, // fd
                    i + 10); // port
        }

        PortPair pp = rp.allocate(1, 2);
        ::mcassert(pp.userport == 10);
        ::mcassert(pp.userfd == 1);
        ::mcassert(pp.devport == 10);
        ::mcassert(pp.devicefd == 1);

        // 同一个user-dev, 返回相同的通道
        PortPair pp2 = rp.allocate(1, 2);
        ::mcassert(pp2.userport == 10);
        ::mcassert(pp2.userfd == 1);
        ::mcassert(pp2.devport == 10);
        ::mcassert(pp2.devicefd == 1);

        // 一个端口中不可以有相同的设备id
        PortPair pp3 = rp.allocate(2, 2);
        ::mcassert(pp3.userport == 10); // userport 总是同一个
        ::mcassert(pp3.userfd == 1);
        ::mcassert(pp3.devport == 11); // devport变成下一个
        ::mcassert(pp3.devicefd == 2);


        // 不同的设备id
        PortPair pp5 = rp.allocate(1, 3);
        ::mcassert(pp5.userport == 10);
        ::mcassert(pp5.userfd == 1);
        ::mcassert(pp5.devport == 10);
        ::mcassert(pp5.devicefd == 1);

        mcassert(rp.m_channels.size() == 3);
        mcassert(rp.m_ports[10].deviceids.size() == 2);

        // 己经分配完了, 这次要失败
        PortPair pp4 = rp.allocate(3, 2);
        ::mcassert(pp4.userport == 10); // userport 总是同一个
        ::mcassert(pp4.userfd == 1);
        ::mcassert(!pp4);
        ::mcassert(pp4.devport == 0); // devport变成下一个
        ::mcassert(pp4.devicefd == -1);

        // 释放 2-2, 它们占用端口: 10, 11
        rp.release(2, 2, pp3.channelid);
        mcassert(rp.m_channels.size() == 2);
        mcassert(rp.m_ports[11].deviceids.size() == 0);
        // 这次可以成功了
        PortPair pp6 = rp.allocate(3, 2);
        ::mcassert(pp6.userport == 10); // userport 总是同一个
        ::mcassert(pp6.userfd == 1);
        ::mcassert(pp6.devport == 11); // devport使用上一次释放的端口11
        ::mcassert(pp6.devicefd == 2);

        ::writelog("OK test RelayPorts\n");
    }
    catch (std::string info)
    {
        ::writelog(InfoType, "Error %s", info.c_str());
        ::writelog("Failed test RelayPorts\n");
    }
}

