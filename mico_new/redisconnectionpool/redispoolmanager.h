#ifndef REDIS_POOL_MANAGER__H__
#define REDIS_POOL_MANAGER__H__
#include <stdint.h>
#include <string>
#include <unordered_map>
#include "thread/mutexwrap.h"

class RedisConnectionPool;
class RedisPoolManager
{
public:
    RedisConnectionPool *get(const std::string &ip, uint16_t port, const std::string &pwd);

private:
    std::unordered_map<std::string, RedisConnectionPool *> m_redispool;
    MutexWrap mutex;
};
#endif

