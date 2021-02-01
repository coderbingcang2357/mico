#include "./testreadallonlinefromredis.h"
#include "../onlineinfo/userdata.h"
#include "../onlineinfo/devicedata.h"
#include "../onlineinfo/rediscache.h"
#include "util/util.h"

void testReadAllOnlineFromRedis()
{
    auto clients = RedisCache::instance()->getAll();
    for (auto &c : clients)
    {
        printf("%lu, %s, %d, %s\n", c->id(), c->name().c_str(), c->loginServerID(), ::timet2String(c->loginTime()).c_str());
    }
}
