#include "udpsession.h"
//static const uint16_t MAX_ID_CNT = 100;

//Udpsession::Udpsession(uint64_t _clientid, const sockaddr_in &_addrc)
//    : clientid(_clientid), addrclient(_addrc)
//{
//    gettimeofday(&lasttime, 0);
//}
//
//void Udpsession::insertID(uint16_t id)
//{
//    if (idset.find(id) != idset.end())
//        return;
//
//    idlist.push_back(id);
//    idset.insert(id);
//    // 超过范围把最前面的删除
//    if (idlist.size() > MAX_ID_CNT)
//    {
//        auto idtodel = idlist.front();
//        idlist.pop_front();
//        idset.erase(idtodel);
//    }
//}
//
//bool Udpsession::isIDExist(uint16_t id)
//{
//    return idset.find(id) != idset.end();
//}

