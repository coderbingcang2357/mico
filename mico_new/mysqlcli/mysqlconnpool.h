#ifndef MYSQL_CONN_POOL__H
#define MYSQL_CONN_POOL__H
#include <list>
#include "thread/mutexwrap.h"
#include "imysqlconnpool.h"
#include "sqlconnpara.h"
class MysqlConnection;

class MysqlConnPool : public IMysqlConnPool
{
public:
    MysqlConnPool(int maxconn=100, const SqlParam &par = SqlParam());
	~MysqlConnPool();
    static MysqlConnection* getConnection();
    static void release(MysqlConnection *);
    std::shared_ptr<MysqlConnection> get() override;

private:
    void releaseConn(MysqlConnection *c);

    uint32_t m_maxConn;
    MutexWrap m_setmutex;
    std::list<MysqlConnection*> m_connset;
    SqlParam m_connpara;
};

#endif
