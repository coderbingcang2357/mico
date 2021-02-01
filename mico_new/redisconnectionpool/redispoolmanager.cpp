#include "redispoolmanager.h"
#include "redisconnectionpool.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"

RedisConnectionPool *RedisPoolManager::get(const std::string &ip, uint16_t port, const std::string &pwd)
{
    char buf[1024];
    snprintf(buf, sizeof(buf),
        "%s:%d",
        ip.c_str(),
        port);
    std::string ipport(buf);
    {
        MutexGuard g(&mutex);
        auto it = m_redispool.find(ipport);
        if (it != m_redispool.end())
        {
            return it->second;
        }
    }

    RedisConnectionPool *p = new RedisConnectionPool(ip, port, pwd);
    MutexGuard g(&mutex);
    m_redispool.insert(std::make_pair(ipport, p));
    return p;
}

