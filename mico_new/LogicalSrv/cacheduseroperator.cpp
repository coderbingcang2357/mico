#include "dbproxyclient/dbproxyclientpool.h"
#include "dbproxyclient/dbproxyclient.h"
#include "dbproxyclient/cacheddbuseroperator.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
//#include "dbproxy/dbproxy"
#include "cacheduseroperator.h"
#include "user/user.h"
#include "config/configreader.h"
#include "protocoldef/protocol.h"

static DbproxyClientPool *pool;
static MutexWrap poollock;
static DbproxyClientPool *getpool()
{
    if (pool == 0)
    {
        Configure *c = Configure::getConfig();
        DbproxyClientPool *tmp = new DbproxyClientPool(c->dbproxy_addr, c->dbproxy_port);

        MutexGuard g(&poollock);
        pool = tmp;
    }
    return pool;
}

CachedUserOperator::CachedUserOperator()
{
}

int CachedUserOperator::getUserPassword(uint64_t userid, std::string *password)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(userid, &u, &answercode) == DbSuccess)
    {
        if (answercode == ANSC::SUCCESS)
        {
            *password = u.info.pwd;
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

int CachedUserOperator::getUserPassword(const std::string &usraccount, std::string *password)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(usraccount, &u, &answercode) == DbSuccess)
    {
        if (answercode == ANSC::SUCCESS)
        {
            *password = u.info.pwd;
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
}

bool CachedUserOperator::isUserExist(const std::string &usraccount)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(usraccount, &u, &answercode) == DbSuccess)
    {
        if (answercode == ANSC::SUCCESS)
            return true;
        else
            return false;
        // *password = u.info.pwd;
    }
    else
    {
        return false;
    }

    return true;
}

UserInfo *CachedUserOperator::getUserInfo(uint64_t userid)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(userid, &u, &answercode) == DbSuccess)
    {
        if (answercode != ANSC::SUCCESS)
            return 0;

        UserInfo *ui = new UserInfo;
        ui->userid = userid;
        ui->account = u.info.account;
        ui->mail = u.info.mail;
        ui->nickName = u.info.nickName;
        ui->signature = u.info.signature;
        ui->phone = u.info.phone;
        ui->headNumber = u.info.headNumber;
        ui->pwd = u.info.pwd;

        return ui;
    }
    else
    {
        return 0;
    }
}

UserInfo *CachedUserOperator::getUserInfo(const std::string &useraccount)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(useraccount, &u, &answercode) == DbSuccess)
    {
        if (answercode != ANSC::SUCCESS)
            return 0;

        UserInfo *ui = new UserInfo;
        ui->userid = u.info.userid;
        ui->account = useraccount;
        ui->mail = u.info.mail;
        ui->nickName = u.info.nickName;
        ui->signature = u.info.signature;
        ui->phone = u.info.phone;
        ui->headNumber = u.info.headNumber;
        ui->pwd = u.info.pwd;

        return ui;
    }
    else
    {
        return 0;
    }

}

int CachedUserOperator::getUserIDByAccount(const std::string &usraccount, uint64_t *userid)
{
    std::shared_ptr<DbproxyClient> client = getpool()->get();
    CachedDbUserOperator cacheuserdb(client.get());
    uint16_t answercode = ANSC::SUCCESS;
    User u;
    if (cacheuserdb.getUserInfo(usraccount, &u, &answercode) == DbSuccess)
    {
        if (answercode != ANSC::SUCCESS)
            return -1;

        *userid = u.info.userid;
        return 0;
    }
    else
    {
        return -1;
    }
}

