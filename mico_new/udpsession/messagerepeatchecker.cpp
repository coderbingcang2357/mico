#include "messagerepeatchecker.h"
#include <string.h>
#include "thread/mutexguard.h"
#include <assert.h>
#include "udpsession.h"

//bool operator < (const MessageRepeatCheaker::UdpconnKey &l, const MessageRepeatCheaker::UdpconnKey &r)
//{
//    if (l.iplow < r.iplow)
//        return true;
//    else if (l.iplow > r.iplow)
//        return false;
//    else if (l.iphigh < r.iphigh)
//        return true;
//    else if (l.iphigh > r.iphigh)
//        return false;
//    else if (l.port < r.port)
//        return true;
//    else if (l.port > r.port)
//        return false;
//    else if (l.clientid < r.clientid)
//        return true;
//    else if (l.clientid > r.clientid)
//        return false;
//    else
//        return false;
//}
//
//// !!! 非线程安全, 这个函数不可以在多线程中同时执行, 因为
//// isIDExist和insertID是线程不安全的
//bool MessageRepeatCheaker::isMessageRepeat(sockaddr *addr,
//        uint64_t clientid,
//        uint16_t msgSerialNumber, uint16_t cmd)
//{
//    sockaddr_in inaddr = *(sockaddr_in *)addr;
//    UdpconnKey key;
//    key.iplow = inaddr.sin_addr.s_addr;
//    key.port = inaddr.sin_port;
//    key.clientid = clientid;
//
//    std::shared_ptr<Udpsession> us;
//
//    {
//        MutexGuard g(&lock);
//        auto it = m_udpconnections.find(key);
//        if (it != m_udpconnections.end())
//        {
//            us = it->second;
//        }
//        else
//        {
//            // 因为这个检查只有在建立连接后才检查, 所以应该要一定可以找到
//            assert(false);
//            return false;
//        }
//    }
//
//    // 序列号不存在, 则不是重发包, 保存序列号
//    if (!us->isIDExist(msgSerialNumber))
//    {
//        // update timer
//        gettimeofday(&(us->lasttime), 0);
//        us->insertID(msgSerialNumber);
//        return false;
//    }
//    // 存在则是重发包
//    else
//    {
//        return true;
//    }
//}
//
//void MessageRepeatCheaker::createSession(sockaddr *addr, uint64_t clientid)
//{
//    sockaddr_in inaddr = *(sockaddr_in *)addr;
//    std::shared_ptr<Udpsession> us = std::make_shared<Udpsession>(clientid, inaddr);
//    UdpconnKey key;
//    key.iplow = inaddr.sin_addr.s_addr;
//    key.port = inaddr.sin_port;
//    key.clientid = clientid;
//
//    MutexGuard g(&lock);
//    auto it = m_udpconnections.find(key);
//    if (it != m_udpconnections.end())
//    {
//        m_udpconnections.erase(it);
//    }
//    m_udpconnections.insert(std::make_pair(key, us));
//}
//
//void MessageRepeatCheaker::removeSession(sockaddr *addr, uint64_t clientid)
//{
//    sockaddr_in inaddr = *(sockaddr_in *)addr;
//    std::shared_ptr<Udpsession> us = std::make_shared<Udpsession>(clientid, inaddr);
//    UdpconnKey key;
//    key.iplow = inaddr.sin_addr.s_addr;
//    key.port = inaddr.sin_port;
//    key.clientid = clientid;
//
//    MutexGuard g(&lock);
//    auto it = m_udpconnections.find(key);
//    if (it != m_udpconnections.end())
//    {
//        m_udpconnections.erase(it);
//    }
//}
