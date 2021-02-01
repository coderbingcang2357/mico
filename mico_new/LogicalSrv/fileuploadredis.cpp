#include <string.h>
#include "util/logwriter.h"
#include "fileuploadredis.h"
#include "thread/mutexguard.h"
#include "redisconnectionpool/redisconns.h"

FileUpAndDownloadRedis FileUpAndDownloadRedis::getInstance(
    const std::string &ip,
    uint16_t port,
    const std::string &password)
{
    FileUpAndDownloadRedis fr;
    fr.rediscontext = RedisConn::getRedisConn(ip, port, password);

    return fr;
}

bool FileUpAndDownloadRedis::insertUploadUrl(const std::string &uid,
    uint64_t sceneid,
    const std::string &filename,
    uint32_t filesize)
{
    redisContext *c = rediscontext.get();
    if (c == 0)
        return false;

    // write to redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                             "rpush %s %lu %s %u", // uid sceneid filename file size
                             uid.c_str(),
                             sceneid,
                             filename.c_str(),
                             filesize);

    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (c->err == REDIS_ERR_IO)
        {
            //
            ::writelog(InfoType, "io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                ::writelog("redis conection closed.\n");
            }
        }
        else
        {
            ::writelog(InfoType, "redis command error: %s\n", c->errstr);
        }
        return false;
    }
    else // ok
    {
        //.
    }
    if (reply != 0)
        freeReplyObject(reply);
    return true;
}

bool FileUpAndDownloadRedis::deleteUploadUrl(const std::string &uid)
{
    //redisContext *c = rediscontext.get();
    return true;
}

bool FileUpAndDownloadRedis::insertDownloadUrl(const std::string &uid,
    uint64_t sceneid,
    const std::string &filename)
{
    redisContext *c = rediscontext.get();
    if (c == 0)
        return false;

    // write to redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                             "hset downfileinfo %s %lu/%s", // sceneid, filename
                             uid.c_str(),
                             sceneid,
                             filename.c_str());

    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (c->err == REDIS_ERR_IO)
        {
            //
            ::writelog(InfoType, "io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                ::writelog("redis conection closed.\n");
            }
        }
        else
        {
            ::writelog(InfoType, "redis command error: %s\n", c->errstr);
        }

        return false;
    }
    else // ok
    {
        //.
    }
    if (reply != 0)
        freeReplyObject(reply);
    return true;
}

bool FileUpAndDownloadRedis::postDelSceneMessage(const std::string &msg)
{
    redisContext *c = rediscontext.get();
    if (c == 0)
        return false;

    // write to redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c,
                             "rpush delscene %s", // sceneid, filename
                             msg.c_str());

    if (reply == 0 || REDIS_REPLY_ERROR == reply->type)
    {
        // error
        if (c->err == REDIS_ERR_IO)
        {
            //
            ::writelog(InfoType, "io error:%s\n", strerror(errno));
            if (errno == EPIPE)
            {
                // connection closed
                ::writelog("redis conection closed.\n");
            }
        }
        else
        {
            ::writelog(InfoType, "redis command error: %s\n", c->errstr);
        }

        return false;
    }
    else // ok
    {
        //.
    }
    if (reply != 0)
        freeReplyObject(reply);
    return true;

}

bool FileUpAndDownloadRedis::deleteDownloadUrl(const std::string &uid)
{
    // redisContext *c = rediscontext.get();
    return true;
}

