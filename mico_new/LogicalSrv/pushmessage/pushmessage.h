#ifndef __PUSHMESSAGE__H__
#define __PUSHMESSAGE__H__
#include <stdint.h>
#include <list>
#include <string>

class IMysqlConnPool;
class PushMessage
{
public:
    operator bool ()
    {
        return id != 0;
    }
    uint64_t id = 0;
    std::string createdate;
    std::string message;
};

class PushMessageDatabase
{
public:
    PushMessageDatabase(IMysqlConnPool *dbconn);
    // get user's pushmessage
    std::list<PushMessage> get(uint64_t userid);
    // a push message has accepted by user, insert a record to database
    void messageAck(uint64_t userid, uint64_t msgid);
    // manager has inserted a new push message to database, msgid is pushmessage's
    // id
    // we read it from database and send to all online user
    PushMessage getPushMessage(uint64_t msgid);

    void test();

private:
    IMysqlConnPool *m_mysqlpool;
};
#endif
