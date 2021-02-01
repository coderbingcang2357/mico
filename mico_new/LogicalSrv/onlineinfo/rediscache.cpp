#include <assert.h>
#include <errno.h>
#include <string.h>
#include "protocoldef/protocol.h"
#include "util/logwriter.h"
#include "userdata.h"
#include "devicedata.h"
#include "config/configreader.h"
#include "rediscache.h"
#include "redisconnectionpool/redisconnectionpool.h"
#include "util/util.h"

//-----------------------------------------------------------------------------
RedisCache::RedisCache()
{
    Configure *cfg = Configure::getConfig();
    pool = new RedisConnectionPool(cfg->redis_address,
                    cfg->redis_port,
                    cfg->redis_pwd);
}

RedisCache *RedisCache::instance()
{
    static RedisCache p;
    return &p;
}

RedisCache::~RedisCache()
{
    delete pool;
}

bool RedisCache::objectExist(uint64_t objid)
{
    std::shared_ptr<redisContext> ptrc = pool->get();
    redisContext *c = ptrc.get();

    if (!c)
        return false;

    bool ishere = false;
    // find in redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                             "HEXISTS online_info %lu",
                             objid);

    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (c->err == REDIS_ERR_IO)
        {
            //
            printf("io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                printf("redis conection closed.\n");
            }
        }
        else
        {
            printf("redis command error: %s\n", c->errstr);
        }
        // release c and try to reconnect
    }
    else // ok
    {
        //.
        if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1)
        {
            ishere = true;
        }
        else
        {
            ishere = false;
        }
    }
    if (reply != 0)
        freeReplyObject(reply);

    return ishere;

}

void RedisCache::insertObject(uint64_t objid, const shared_ptr<UserData> &data)
{
    std::string json = data->toStringJson();
    insert(objid, json);
}

void RedisCache::insertObject(uint64_t objid, const shared_ptr<DeviceData> &data)
{
    std::string json = data->toStringJson();
    insert(objid, json);
}

void RedisCache::insert(uint64_t objid,
                const std::string &json)
{
    // 添加到redis中
    std::shared_ptr<redisContext> ptrc = pool->get();
    redisContext *c = ptrc.get();
    if (c)
    {
        // write to redis
        redisReply *reply;
        reply = (redisReply *)redisCommand(c,
                                 "hset online_info %lu %s",
                                 objid, json.c_str());

        if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
        {
            // error
            if (c->err == REDIS_ERR_IO)
            {
                //
                printf("io error:%s\n", strerror(errno));
                if (errno == EPIPE)
                {
                    // connection closed
                    printf("redis conection closed.\n");
                }
            }
            else
            {
                printf("redis command error: %s\n", c->errstr);
            }
        }
        else // ok
        {
            //.
        }
        if (reply != 0)
            freeReplyObject(reply);
    }
}

void RedisCache::removeObject(uint64_t objid)
{
    std::shared_ptr<redisContext> ptrc = pool->get();
    redisContext *c = ptrc.get();
    // 从redis中删除
    if (c)
    {
        redisReply *reply;
        reply = (redisReply *)redisCommand(c,
                                 "hdel online_info %lu", objid);
        if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
        {
            // error
            if (c->err == REDIS_ERR_IO && errno == EPIPE)
            {
                //
                printf("io error:%s\n", strerror(errno));
                printf("redis conection closed.\n");
                printf("redis command error: %s\n", c->errstr);
            }
            else
            {
                printf("redis command error: %s\n", c->errstr);
            }
        }
        else // ok
        {
            //.
            if (reply->type != REDIS_REPLY_INTEGER || reply->integer != 1)
                printf("redis del failed: %lu\n", objid);

            ::writelog(InfoType, "del online_info %lu", objid);
        }
        if (reply != 0)
            freeReplyObject(reply);
    }
    else
    {
        printf("remove failed: no connection.\n");
    }
}

shared_ptr<IClient> RedisCache::getData(uint64_t objid)
{
    std::shared_ptr<redisContext> ptrc = pool->get();
    redisContext *c = ptrc.get();
    if (!c)
        return shared_ptr<IClient>();

    // 从redis中查找
    // get
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                                "hget online_info %lu",
                                objid);
    std::string json;
    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (c->err == REDIS_ERR_IO)
        {
            //
            printf("io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                printf("redis conection closed.\n");
            }
        }
        else
        {
            printf("redis command error: %s\n", c->errstr);
        }
    }
    else // ok
    {
        //.
        if (reply->type == REDIS_REPLY_STRING)
            json = std::string(reply->str, reply->len);
        // else return value is error
    }
    if (reply != 0)
        freeReplyObject(reply);

    if (json.empty())
        return shared_ptr<IClient>();

    IdType idtype = ::GetIDType(objid);
    switch (idtype)
    {
    case IT_User:
    {
        UserData *u = new UserData; //
        if (u->fromStringJson(json))
        {
            return shared_ptr<UserData>(u);
        }
        else
        {
            delete u;
            return shared_ptr<IClient>();
        }
    }
    break;

    case IT_Device:
    {
        DeviceData *d = new DeviceData; //
        if (d->fromStringJson(json))
        {
            return shared_ptr<DeviceData>(d);
        }
        else
        {
            delete d;
            return shared_ptr<IClient>();
        }
    }
    break;

    default:
        ::writelog(InfoType, "error id %ld", objid);
    }

    return shared_ptr<IClient>();
}

std::list<std::shared_ptr<IClient>> RedisCache::getAll()
{
    std::shared_ptr<redisContext> ptrc = pool->get();
    redisContext *c = ptrc.get();
    std::list<shared_ptr<IClient>> result;
    if (!c)
        return result;

    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                                "hgetall online_info");

    if (RedisConnectionPool::hasError(c, reply))
        return result;

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (size_t i = 0; i < reply->elements; i += 2)
        {
            redisReply *keyreply = reply->element[i];
            redisReply *valuereply = reply->element[i+1];
            uint64_t id=0;
            std::string json;
            if (keyreply->type == REDIS_REPLY_INTEGER)
            {
                id = keyreply->integer;
            }
            else if (keyreply->type == REDIS_REPLY_STRING)
            {
                //::writelog(WarningType, "error string reply");
                id = ::StrtoU64(std::string(keyreply->str, keyreply->len).c_str());
            }
            else
            {
                ::writelog(WarningType, "error reply");
                continue;
            }
            if (valuereply->type == REDIS_REPLY_STRING)
            {
                json = std::string(valuereply->str, valuereply->len);
            }
            else
            {
                ::writelog(WarningType, "error value reply");
                continue;
            }

            IdType idtype = ::GetIDType(id);
            switch (idtype)
            {
            case IT_User:
            {
                UserData *u = new UserData; //
                if (u->fromStringJson(json))
                {
                    result.push_back(shared_ptr<UserData>(u));
                }
                else
                {
                    delete u;
                }
            }
            break;

            case IT_Device:
            {
                DeviceData *d = new DeviceData; //
                if (d->fromStringJson(json))
                {
                    result.push_back(shared_ptr<DeviceData>(d));
                }
                else
                {
                    delete d;
                }
            }
            break;

            default:
                ::writelog(InfoType, "read redis error id %ld", id);
            }
        }
    }
    else
    {
        ::writelog(WarningType, "need array reply.");
    }
    freeReplyObject(reply);
    return result;
}
