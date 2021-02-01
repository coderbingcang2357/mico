#include <unistd.h>
#include <string.h>
#include <assert.h>
#include "util/logwriter.h"
#include "util/util.h"
#include "config/configreader.h"
#include "redisconnectionpool/redisconnectionpool.h"

void processDelSceneMessage(redisReply *reply);
void rmscene(const std::string &sceneid, const std::string &userid);
bool runasdaemon = false;
bool isrun = true;
const char *filedir;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        ::writelog("error argument\n");
        ::writelog(InfoType, "usage: %s path [-daemon]\n", argv[0]);
        return 1;
    }

    filedir = argv[1];

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
            runasdaemon = true;
        else
            printf("Del scene Server: Unknow parameter: %s.\n", argv[i]);
    }

    char configpath[1024];
    Configure *config = Configure::getConfig();
    Configure::getConfigFilePath(configpath, sizeof(configpath));

    if (config->read(configpath) != 0)
    {
        exit(-1);
    }

    if (config->serverid < 0)
    {
        printf("error serverid");
        return 0;
    }

    logSetPrefix("Del scene Server");
    if (runasdaemon)
    {
        loginit(LogPathLogical);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    RedisConnectionPool pool("127.0.0.1",
        config->file_redis_port,
        config->redis_pwd);
    redisReply *reply;
    for (;isrun;)
    {
        std::shared_ptr<redisContext> conn = pool.get();

        if (!conn)
        {
            ::writelog(InfoType, "delscene failed: getconn failed.%s");
            ::sleep(10);
            continue;
        }

        redisContext *c = conn.get();

        reply = (redisReply *)redisCommand(c,
                                "blpop delscene 5"); // 5秒超时
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
            if (reply->type == REDIS_REPLY_NIL)
            {
                //::writelog("time out.1");
            }
            else if (reply->type == REDIS_REPLY_ARRAY)
            {
                processDelSceneMessage(reply);
            }
            else
            {
                assert(false);
            }
        }
        if (reply != 0)
            freeReplyObject(reply);

    }
    return 0;
}

void processDelSceneMessage(redisReply *reply)
{
    std::string path;
    assert(reply->elements == 2);
    redisReply *res = reply->element[1];
    switch (res->type)
    {
    case REDIS_REPLY_INTEGER:
    {
        ::writelog(InfoType, "Integer: delscene: %lu", res->integer);
        path.append(filedir);
        path.append("/");
        path.append(std::to_string(res->integer));
    }
    break;

    case REDIS_REPLY_STRING:
    {
        ::writelog(InfoType, "str: delscene: %s", res->str);
        std::string scenedir(res->str);
        size_t dotpos = scenedir.find(".");
        if (dotpos != std::string::npos)
        {
            ::writelog(InfoType, "delscene failed: error scene name.%s", res->str);
            return;
        }
        std::vector<std::string> strlist;
        splitString(scenedir, ":", &strlist);
        if (strlist.size() == 2)
        {
            rmscene(strlist[0], // 场景ID
                strlist[1]); // 用户ID
        }
        else
        {
            ::writelog(InfoType, "rm scene failed, error scenename:%s", res->str);
        }
    }
    break;

    default:
        assert(false);
        return;
    }
}

void rmscene(const std::string &sceneid, const std::string &userid)
{
    std::string old(filedir);
    old.append("/");
    old.append(sceneid);

    std::string newname(filedir);
    newname.append("/");
    newname.append(sceneid);
    newname.append(".");
    newname.append(userid);
    newname.append(".del");

    if (rename(old.c_str(), newname.c_str()) != 0)
    {
        ::writelog(InfoType, "!!delscene failed: %s:%s", sceneid.c_str(), userid.c_str());
    }
    else
    {
        ::writelog(InfoType, "delscene Success: %s:%s", sceneid.c_str(), userid.c_str());
    }
}

