#include <assert.h>
#include <unistd.h>
#include "pushmessagesubscribethread.h"
#include "config/configreader.h"
#include "../onlineinfo/mainthreadmsgqueue.h"
#include "redisconnectionpool/redisconnectionpool.h"
#include "util/logwriter.h"
#include "Message/Message.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "../newmessagecommand.h"

using namespace rapidjson;

PushMessageSubscribeThread::PushMessageSubscribeThread()
{
    m_isrun = true;
    m_th = new Thread([this](){
        Configure *cfg = Configure::getConfig();
        RedisConnectionPool pool(cfg->redis_address, 
                                    cfg->redis_port,
                                    cfg->redis_pwd);
        for (;m_isrun;)
        {
            auto ctx = pool.get();
            if (!ctx)
            {
                // noconnect
                assert(false);
                ::sleep(4); // sleep 2 sec and try again
                continue;
            }
            // cmd timeout 100 ms
            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100 * 1000; // 100 ms
            redisSetTimeout(ctx.get(), tv);
            redisReply *subreply = (redisReply *)redisCommand(ctx.get(), "SUBSCRIBE pushmsg");
            RedisReplyGuard g(subreply );

            if (RedisConnectionPool::hasError(ctx.get(), subreply))
            {
                // >>>>>>
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
            }
            else
            {
                //
                assert(false);
                continue;
            }

            for (;m_isrun;)
            {
                redisReply *reply = NULL;
                RedisReplyGuard greply(reply);
                if (redisGetReply(ctx.get(), (void **)&reply) == REDIS_OK)
                {
                    assert(reply->type == REDIS_REPLY_ARRAY);
                    assert(reply->elements == 3);
                    redisReply *d = reply->element[2];
                    std::string json(d->str, d->len);
                    ::writelog(InfoType, "get: %s", json.data());

                    Document doc;
                    doc.Parse(json.c_str());
                    assert(doc.IsObject());

                    uint64_t notifyid = 0;
                    if (doc.HasMember("notifyid"))
                    {
                        notifyid = doc["notifyid"].GetUint64();

                        InternalMessage *imsg = new InternalMessage(new Message);
                        Message *msg = imsg->message();
                        msg->appendContent(notifyid);
                        msg->setCommandID(CMD::IN_CMD_NEW_PUSH_MESSAGE);
                        MainQueue::postCommand(new NewMessageCommand(imsg));
                    }
                    else
                    {
                        assert(false);
                    }
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
                        break;
                    }
                }
            }

        }
    });
}

PushMessageSubscribeThread::~PushMessageSubscribeThread()
{
    m_isrun = false;
    delete m_th;
}
