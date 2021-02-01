#include <assert.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include "passworddao.h"
#include "redisconnectionpool/redisconns.h"
#include "config/configreader.h"
#include "util/logwriter.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "util/util.h"

int PasswordDao::getPasswordAndID(const std::string &account,
                    uint64_t *id,
                    std::string *pwd)
{
    // ok in redis
    if (getPasswordAndIDFromRedis(account, id, pwd) == 0)
    {
        return 0;
    }
    // read from database
    if (getPasswordAndIDFromMysql(account, id, pwd) == 0)
    {
        // ok, save to redis
        savePasswordAndIdToRedis(account, *id, *pwd);
        return 0;
    }
    else
    {
        return -1;
    }
}

int PasswordDao::getPasswordAndIDFromRedis(const std::string &account,
                    uint64_t *id,
                    std::string *pwd)
{
    return -1;
    Configure *c = Configure::getConfig();
    std::shared_ptr<redisContext> redis =
        RedisConn::getRedisConn(c->redis_address, c->redis_port, c->redis_pwd);
    assert(redis);
    std::string account_key;
    account_key.append(account);
    account_key.append("_pwd");
    redisContext *redisctx = redis.get();
    redisReply *r = (redisReply *)redisCommand(redisctx, "get %s",
                                        account_key.c_str());
    // for auto del
    std::unique_ptr<redisReply> __rdel(r);
    // failed
    if (RedisConnectionPool::hasError(redisctx, r))
    {
        return -1;
    }
    // ok
    if (r->type == REDIS_REPLY_STRING)
    {
        char *start = r->str;
        char *end = r->str + r->len;
        auto pos = std::find(start, end, ':');
        if (pos == end)
        {
            ::writelog("get pwd error value");
            return -1;
        }
        *pos = '\0';
        // ....id
        *id = strtoull(start, 0, 10);
        std::string pwdtmp(pos + 1);
        if (pwdtmp.size() != 32)
        {
            ::writelog("get pwd error pwd len");
            return -1;
        }
        *pwd = pwdtmp;
        return 0;
    }
    else if (r->type == REDIS_REPLY_NIL)
    {
        return -1;
    }
    else
    {
        ::writelog("get pwd unknown relay.");
        assert(false);
        return -1;
    }

}

int PasswordDao::getPasswordAndIDFromMysql(const std::string &account, uint64_t *id, std::string *pwd)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
    sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
            MysqlConnPool::release(p);
        });
    if(!sql)
        return -1;

    char buf[1024];
    int len = sql->RealEscapeString(buf,
                const_cast<char*>(account.c_str()),
                account.size());
    assert(uint64_t(len) < sizeof(buf));
    std::string useraccountAfterEscape(buf, len);
    char bufsql[1024];
    snprintf(bufsql, sizeof(bufsql),
        "select ID, password from T_User where account='%s'",
        useraccountAfterEscape.c_str());

    int result = sql->Select(std::string(bufsql));
    if(result == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());

        *id= StrtoU64(row[0]);
        *pwd = GetString(row[1]);

        assert(pwd->size() == 32);
        return 0;
    }
    else
    {
        return -1;
    }
}

int PasswordDao::savePasswordAndIdToRedis(const std::string &account,
            uint64_t id,
            const std::string &pwd)
{    Configure *c = Configure::getConfig();
    std::shared_ptr<redisContext> redis =
        RedisConn::getRedisConn(c->redis_address, c->redis_port, c->redis_pwd);
    assert(redis);
    std::string account_key;
    account_key.append(account);
    account_key.append("_pwd");
    char value[128];
    snprintf(value, sizeof(value), "%lu:%s", id, pwd.c_str());
    redisContext *redisctx = redis.get();
    redisReply *r = (redisReply *)redisCommand(redisctx, "set %s %s",
                                account_key.c_str(),
                                value);
    // print error log
    RedisConnectionPool::hasError(redisctx, r);
    if (r != 0)
    {
        freeReplyObject(r);
    }
    return 0;
}

// 0 success
int PasswordDao::modifyPassword(const std::string &account,
                uint64_t uid,
                const std::string &newpwd)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
        sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
                MysqlConnPool::release(p);
            });
    if(!sql)
        return -1;

    assert(newpwd.size() == 32);

    char buf[1024];
    snprintf(buf, sizeof(buf),
            "update T_User set password='%s' where ID=%lu",
            newpwd.c_str(), 
            uid);
    //string statement = EMPTY_STRING
    //       + "UPDATE " + TUSER
    //       + " SET " + TUSER__PWD + "=" + "\'" + newpassword + "\'"
    //       + " WHERE " + TUSER__ID + "=" + "\'" + Uint2Str(userid) + "\';";

    // 0 success, otherwish failed
    int r = sql->Update(std::string(buf));
    // r<0表示sql执行失败
    if (r < 0)
    {
        return -1;
    }
    else
    {
        // update pwd cache from redis
        // r == 0表示更新成功, r==1表示两次密码一样
        if (r == 0 && savePasswordAndIdToRedis(account, uid, newpwd) != 0)
        {
            ::writelog(InfoType,
                        "modify pwd, save pwd to reids failed.%s",
                        account.c_str());
        }
        return 0;
    }
}
