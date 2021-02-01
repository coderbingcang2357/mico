#ifndef REDISCONNECTIONPOOL__H
#define REDISCONNECTIONPOOL__H
#include <string>
#include <list>
#include <memory>
#include <stdint.h>
#include "hiredis/hiredis.h"
#include "thread/mutexwrap.h"
#include "thread/conditionwrap.h"

class RedisConnectionPool
{
public:
    RedisConnectionPool(const std::string &ip, uint16_t port, const std::string &pwd);
    ~RedisConnectionPool();
    std::shared_ptr<redisContext> get();
    int connectionSize() {return conns.size();}

    static bool hasError(redisContext *c, redisReply *r);
    static void freeRedisReply(redisReply *r);
private:
    redisContext *connectToRedis();
    bool isConnected(redisContext *);
    bool getAuth(redisContext *c);

    std::list<redisContext *> conns;
    std::string m_ip;
    uint16_t m_port;
    std::string m_pwd;
    MutexWrap  mutex;
    ConditionWrap cond;
};

class RedisReplyGuard
{
public:
    RedisReplyGuard(redisReply *r);
    ~RedisReplyGuard();

private:
    redisReply *m_r;
};

#endif

