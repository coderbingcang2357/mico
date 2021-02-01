#ifndef MESSAGEREPEATCHECKER__H
#define MESSAGEREPEATCHECKER__H
#include "imessagerepeatchecker.h"
#include <map>
#include <memory>
#include "thread/mutexwrap.h"
#include <string.h>

//class Udpsession;

//class MessageRepeatCheaker : public IMessageRepeatChecker
//{
//public:
//    class UdpconnKey
//    {
//    public:
//        UdpconnKey() : iplow(0), iphigh(0), port(0), clientid(0)
//        {
//        }
//
//        union {
//            struct {
//                uint64_t iplow;
//                uint64_t iphigh;
//            };
//            char ip[16];
//        };
//        uint16_t port;
//        uint64_t clientid;
//
//        friend bool operator < (const UdpconnKey &l, const UdpconnKey &r);
//    };
//
//    bool isMessageRepeat(sockaddr *addr,
//            uint64_t clientid,
//            uint16_t msgSerialNumber,
//            uint16_t cmd) override;
//    void createSession(sockaddr *addr, uint64_t clientid) override;
//    void removeSession(sockaddr *addr, uint64_t clientid) override;
//
//    // void removeTimeoutConnection();
//
//private:
//    std::map<UdpconnKey, std::shared_ptr<Udpsession>> m_udpconnections;
//    MutexWrap lock;
//
//    friend class TestMessageRepeatCheaker;
//};
#endif
