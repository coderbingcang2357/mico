#include "util/logwriter.h"
#include <assert.h>
#include "redisconnectionpool/redisconnectionpool.h"

int main(int , char **)
{
    RedisConnectionPool pool("10.1.240.39", 6379, "3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb");

    for (;;)
    {
        auto ctx = pool.get();
        if (!ctx)
        {
            // noconnect
            // ???
            assert(false);
            continue;
        }
        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        redisSetTimeout(ctx.get(), tv);
        redisReply *subreply = (redisReply *)redisCommand(ctx.get(), "SUBSCRIBE pushmsg");

        if (RedisConnectionPool::hasError(ctx.get(), subreply))
        {
            // >>>>>>
            freeReplyObject(subreply);
            assert(false);
            continue;
        }
        if (subreply->type == REDIS_REPLY_ARRAY)
        {
            assert(subreply->elements == 3);
            redisReply *tr = subreply->element[0];
            redisReply *chr = subreply->element[1];
            redisReply *cnt = subreply->element[2];
            std::string t(tr->str, tr->len), ch(chr->str, chr->len);
            ::writelog(InfoType, "type: %s, channel: %s, cnt: %d",
                    t.data(), ch.data(), cnt->integer);

            freeReplyObject(subreply);
        }
        else
        {
            //
            assert(false);
            freeReplyObject(subreply);
            continue;
        }

        for (;;)
        {
            redisReply *reply = NULL;
            if (redisGetReply(ctx.get(), (void **)&reply) == REDIS_OK)
            {
                assert(reply->type == REDIS_REPLY_ARRAY);
                assert(reply->elements == 3);
                redisReply *d = reply->element[2];
                ::writelog(InfoType, "get: %s", std::string(d->str, d->len).data());
            }
            else
            {
                //if (RedisConnectionPool::hasError(ctx.get(), reply))
                if (ctx->err == REDIS_ERR_IO && errno == EAGAIN)
                {
                    // timeout.
                    ctx->err = REDIS_OK;
                }
                else
                {
                    assert(false);
                }
            }
            if (reply)
                freeReplyObject(reply);
        }
    }
    return 0;
}
