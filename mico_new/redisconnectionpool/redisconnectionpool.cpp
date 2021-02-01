#include <errno.h>
#include <string.h>
#include <assert.h>
#include "redisconnectionpool.h"
#include "thread/mutexguard.h"
#include "util/logwriter.h"

//-----------------------------------------------------------------------------
RedisConnectionPool::RedisConnectionPool(const std::string &ip, uint16_t port, const std::string &pwd)
    : m_ip(ip), m_port(port), m_pwd(pwd)
{
}

RedisConnectionPool::~RedisConnectionPool()
{
	while(conns.size() != 0){
		redisContext *tmp = conns.back();
		conns.pop_back();
		redisFree(tmp);
	}
    //redisFree(c);
}

std::shared_ptr<redisContext> RedisConnectionPool::get()
//redisContext *RedisConnectionPool::get()
{
    redisContext *c;
    mutex.lock();

    if (conns.size() == 0)
    {
        mutex.release();

        c = connectToRedis();
        // conns.push_back(c);
    }
    else
    {
        //while (conns.size() == 0)
        //{
        //    cond.wait(&mutex);
        //}

        c = conns.back();
        conns.pop_back();

        mutex.release();

        if (!isConnected(c))
        {
            redisFree(c);
            printf("connect break reconnect.\n");
            c = connectToRedis();
        }
    }

    if (c == 0)
    {
        assert(false);
        printf("redis get connection error.\n");
    }

    return std::shared_ptr<redisContext>(c, [this](redisContext *p) {
        MutexGuard g(&(this->mutex));
        assert(p != 0);
        if (p != 0)
        {
            this->conns.push_back(p);
            //this->cond.signal();
        }
    });
}

redisContext *RedisConnectionPool::connectToRedis()
{
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redisContext *connect = redisConnectWithTimeout(m_ip.c_str(),
                                m_port,
                                timeout);
    if (connect == NULL || connect->err)
    {
        if (connect)
        {
            printf("Connection error1: %s\n", connect->errstr);
            redisFree(connect);
        }
        else
        {
            printf("Connection error: can't allocate redis context\n");
        }
        return 0;
    }
    // auth
    if (!getAuth(connect))
    {
        return 0;
    }
    else
    {
        redisEnableKeepAlive(connect);
        return connect;
    }
}

bool RedisConnectionPool::getAuth(redisContext *c)
{
    bool isok = false;
    // write to redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                             "auth %s", m_pwd.c_str());

    if (hasError(c, reply))
    {
        isok = false;
    }
    else // ok
    {
        //.
        static std::string ok("OK");
        //printf("auth reply type: %d\n", reply->type);
        if (std::string(reply->str, reply->len) == ok)
        {
            isok = true;
        }
        else
        {
            printf("redis auth failed.\n");
        }
    }

    if (reply != 0)
        freeReplyObject(reply);

    return isok;
}

bool RedisConnectionPool::isConnected(redisContext *connect)
{
    bool isok = false;
    // write to redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(connect,
                             "ping");

    if (hasError(connect, reply))
    {
        isok = false;
    }
    else // ok
    {
        //.
        static std::string pong("PONG");
        if (reply->type == REDIS_REPLY_STATUS
            && std::string(reply->str, reply->len)  == pong)
        {
            isok = true;
        }
        else
        {
            printf("redis ping failed.\n");
        }
    }

    if (reply != 0)
        freeReplyObject(reply);

    return isok;
}

bool RedisConnectionPool::hasError(redisContext *connect, redisReply *reply)
{
    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (connect->err == REDIS_ERR_IO)
        {
            //
            printf("io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                printf("redis conection closed.\n");
            }
        }
        else if (connect->err == REDIS_ERR_EOF) // server closed connection
        {
            printf("redis command error1: %s\n", connect->errstr);
        }
        else
        {
            printf("redis command error2: %s\n", connect->errstr);
        }

        return true;
    }
    else
    {
        return false;
    }
}

void RedisConnectionPool::freeRedisReply(redisReply *r)
{
    if (r)
        freeReplyObject(r);
}

RedisReplyGuard::RedisReplyGuard(redisReply *r) : m_r(r)
{
}

RedisReplyGuard::~RedisReplyGuard()
{
    if (m_r)
        freeReplyObject(m_r);
}
