#ifndef udpsession__h
#define udpsession__h
struct sockaddr_in;
#include <stdint.h>
#include <netinet/in.h>
#include <set>
#include <list>
#include <sys/time.h>

//class Udpsession
//{
//public:
//    Udpsession(uint64_t _clientid, const sockaddr_in &_addrc);
//    const uint64_t clientid;
//    const sockaddr_in addrclient;
//    timeval lasttime;
//
//    void insertID(uint16_t id);
//    bool isIDExist(uint16_t id);
//
//private:
//    std::list<uint16_t> idlist;
//    std::set<uint16_t> idset;
//
//    friend class  TestUdpsession;
//};
#endif
