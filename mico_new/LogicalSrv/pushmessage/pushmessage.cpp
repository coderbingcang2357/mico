#include <assert.h>
#include "pushmessage.h"
#include "mysqlcli/imysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "util/logwriter.h"

PushMessageDatabase::PushMessageDatabase(IMysqlConnPool *dbop)
    : m_mysqlpool(dbop)
{

}

// get user's pushmessage
std::list<PushMessage> PushMessageDatabase::get(uint64_t userid)
{
    std::list<PushMessage> ret;
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog(WarningType, "PushMessageDatabase::get connfailed");
        return ret;
    }
    const char *query = " select ID, createdate, message from PushMessage "
                        " where shouldsend != 0 and ID not in (select pullmessageid from PushMessageReceiverUser where userid=%lu)";
    char querybuf[1024];
    snprintf(querybuf, sizeof(querybuf),
        query,
        userid);
    int r = sql->Select(querybuf);
    if (r >= 0)
    {
        auto result = sql->result();
        int rowcnt = result->rowCount();
        for (int i = 0; i < rowcnt; ++i)
        {
            PushMessage pm;
            auto row = result->nextRow();
            pm.id = row->getUint64(0);
            pm.createdate = row->getString(1);
            pm.message = row->getString(2);
            ret.push_back(pm);
        }
    }
    return ret;
}

// a push message has accepted by user, insert a record to database
void PushMessageDatabase::messageAck(uint64_t userid, uint64_t msgid)
{
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog(WarningType, "PushMessageDatabase::messageack connfailed");
        return ;
    }
    const char *query = "insert into PushMessageReceiverUser (pullmessageid, userid, state, recvdate) values (%lu, %lu, 1, now());";
    char querybuf[1024];
    snprintf(querybuf, sizeof(querybuf),
        query,
        msgid,
        userid);
    sql->Insert(querybuf);
}

// manager has inserted a new push message to database, msgid is pushmessage's
// id
// we read it from database and send to all online user
PushMessage PushMessageDatabase::getPushMessage(uint64_t msgid)
{
    PushMessage pm;
    auto sql = m_mysqlpool->get();
    if(!sql)
    {
        ::writelog(WarningType, "PushMessageDatabase::messageack connfailed");
        return pm;
    }

    const char *query = "select ID, createdate, message from PushMessage where ID=%lu";
    char querybuf[1024];
    snprintf(querybuf, sizeof(querybuf), 
        query,
        msgid);
    int ret = sql->Select(querybuf);
    if (ret >= 0)
    {
        auto result = sql->result();
        int rowcnt = result->rowCount();
        if (rowcnt == 1)
        {
            auto row = result->nextRow();
            pm.id = row->getUint64(0);
            pm.createdate = row->getString(1);
            pm.message = row->getString(2);
        }
        else
        {
            assert(false);
        }
    }
    return pm;
}

void PushMessageDatabase::test()
{

}
