#include "util/sharedptr.h"
#include "messageencrypt.h"
#include "Message/Message.h"
#include "onlineinfo/userdata.h"
#include "onlineinfo/devicedata.h"
#include "onlineinfo/onlineinfo.h"
#include "onlineinfo/rediscache.h"
#include "util/logwriter.h"

// 如果msg对应的objectid不存在, 则不会加密, 因为找不到它对应的sessionkey嘛
int encryptMessage(Message *msg, ICache *cache)
{
    if(msg->contentLen() == 0)
        return 0;

    uint64_t objectID = msg->objectID();
    std::shared_ptr<IClient> data = cache->getData(objectID);
    if (!data)
        return -2;

    std::vector<char> sessk; // sessionkey
    sessk = data->sessionKey();
    assert(sessk.size() == 16);
    msg->Encrypt(&(sessk[0]));

    return 0;

    //if(GetIdType(objectID) == IT_User)
    //{

    //    shared_ptr<UserData> userData
    //        = static_pointer_cast<UserData>(cache->getData(objectID));

    //    if(!userData)
    //        return -2;

    //    sessk = userData->sessionKey();
    //    assert(sessk.size() == 16);
    //}
    //else
    //{
    //    shared_ptr<DeviceData> deviceData
    //        = static_pointer_cast<DeviceData>(cache->getData(objectID));

    //    if(!deviceData)
    //        return -2;

    //    sessk = deviceData->GetCurSessionKey();
    //    assert(sessk.size() == 16);
    //}

    //msg->Encrypt(&(sessk[0]));
    // return 0;
}

int decryptMsg(Message* msg, ICache *cache)
{
    if(msg->contentLen() == 0)
        return 0;

    uint64_t objectID = msg->objectID();

    std::shared_ptr<IClient> data = cache->getData(objectID);
    if (!data)
    {
        data = RedisCache::instance()->getData(objectID);
        if (!data){
			::writelog(InfoType,"objectID:%llu",objectID);
			::writelog("decryptMsg data error!");
            return -2;
		}
    }
    std::vector<char> sessk = data->sessionKey();
    assert(sessk.size() == 16);
    if(sessk.size() != 16 || msg->Decrypt(&(sessk[0])) < 0)
    {
        ::writelog("User session length error.");
        return -1;
    }
    else
        return 0;
    //CacheManager cacheManager;
    // if(GetIdType(objectID) == IT_User)
    // {
    //     shared_ptr<UserData> userData
    //         = static_pointer_cast<UserData>(cache->getData(objectID));
    //     if(!userData)
    //     {
    //         // ::writelog("Logical: can not find User at cache.");
    //         return -2;
    //     }
    //     std::vector<char> sessk = userData->sessionKey();
    //     assert(sessk.size() == 16);

    //     if(sessk.size() != 16 || msg->Decrypt(&(sessk[0])) < 0)
    //     {
    //         // ::writelog("User session length error.");
    //         return -1;
    //     }
    //     else
    //         return 0;
    // }
    // else
    // {
    //     shared_ptr<DeviceData> deviceData
    //         = static_pointer_cast<DeviceData>(cache->getData(objectID));
    //     if(!deviceData)
    //     {
    //         char log[244];
    //         sprintf(log, "Logical: can not find device. %lu", objectID);
    //         ::writelog(log);
    //         return -2;
    //     }
    //     std::vector<char> sessk = deviceData->GetSessionKey();
    //     // device 可能会同时有两相sesssionkey, 一个新的, 一个旧的
    //     // 先用旧的试, 如果不行再用新的
    //     if(sessk.size() != 16 || msg->Decrypt(&(sessk[0])) < 0)
    //     {
    //         std::vector<char> newsessk = deviceData->getNewSessionKey();
    //         assert(newsessk.size() == 16);
    //         if(msg->Decrypt(&(newsessk[0])) < 0)
    //         {
    //             char log[244];
    //             sprintf(log, "Logical: device decrypt failed %lu", objectID);
    //             ::writelog(log);

    //             return -1;
    //         }
    //         else
    //         {
    //             // 用新的解密成功
    //             return 0;
    //         }
    //     }
    //     else
    //     {
    //         // ok
    //         return 0;
    //     }
    // }
}
