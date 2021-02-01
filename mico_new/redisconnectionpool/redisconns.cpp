#include "redisconns.h"
static RedisPoolManager redispool;

std::shared_ptr<redisContext> RedisConn::getRedisConn(const std::string &ip,
                            uint16_t port,
                            const std::string &pwd)
{
    RedisConnectionPool *p = redispool.get(ip, port, pwd);
    return p->get();
}
