#ifndef REDIS_CONNS__H__
#define REDIS_CONNS__H__
#include "redisconnectionpool/redisconnectionpool.h"
#include "redisconnectionpool/redispoolmanager.h"

namespace RedisConn
{
    std::shared_ptr<redisContext> getRedisConn(const std::string &ip,
                                uint16_t port,
                                const std::string &pwd);
}
#endif

