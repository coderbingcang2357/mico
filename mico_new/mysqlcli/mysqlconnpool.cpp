#include <list>
#include <assert.h>
#include "thread/mutexwrap.h"
#include "mysqlconnpool.h"
#include "mysqlconnection.h"
#include "config/configreader.h"
#include "util/logwriter.h"

static MutexWrap setmutex;
static std::list<MysqlConnection*> connset;

MysqlConnection* MysqlConnPool::getConnection()
{
    MysqlConnection *ret = 0;
    setmutex.lock();
    if (connset.size() > 0)
    {
        ret = connset.back();
        connset.pop_back();

        setmutex.release();
        // 还要测试连接的可用性, 如果不可用就要进行重连

    }
    else
    {
        setmutex.release();

        // should i create one ?
        MysqlConnection *c = new MysqlConnection;
        Configure *cfg = Configure::getConfig();
        SqlParam p;
        p.host = cfg->databasesrv_sql_host;
        p.port = cfg->databasesrv_sql_port;
        p.user = cfg->databasesrv_sql_user;
        p.passwd = cfg->databasesrv_sql_pwd;
        p.db = cfg->databasesrv_sql_db;
        if (c->connect(p))
        {
            ret = c;
        }
    }

    return ret;
}

void MysqlConnPool::release(MysqlConnection *c)
{
    setmutex.lock();
    assert(c != 0);
    // 最多保存250个连接
    if (connset.size() >= 250)
    {
        delete c;
    }
    else
    {
        connset.push_front(c);
    }
    setmutex.release();
}

MysqlConnPool::MysqlConnPool(int maxconn, const SqlParam &par)
    : m_maxConn(maxconn), m_connpara(par)
{

}

MysqlConnPool::~MysqlConnPool(){
	while(m_connset.size() != 0){
		MysqlConnection *tmp = m_connset.back(); 
		m_connset.pop_back();
		delete tmp;
	}
}
std::shared_ptr<MysqlConnection> MysqlConnPool::get()
{
    MysqlConnection *ret = 0;
    m_setmutex.lock();
    if (m_connset.size() > 0)
    {
        ret = m_connset.back();
        m_connset.pop_back();

        m_setmutex.release();
        // 还要测试连接的可用性, 如果不可用就要进行重连

    }
    else
    {
        m_setmutex.release();

        // should i create one ?
        MysqlConnection *c = new MysqlConnection;
        SqlParam p = m_connpara;
        if (!p.isValid())
        {
            Configure *cfg = Configure::getConfig();
            p.host = cfg->databasesrv_sql_host;
            p.port = cfg->databasesrv_sql_port;
            p.user = cfg->databasesrv_sql_user;
            p.passwd = cfg->databasesrv_sql_pwd;
            p.db = cfg->databasesrv_sql_db;
        }
        if (c->connect(p))
        {
            ret = c;
        }
    }

    return std::shared_ptr<MysqlConnection>(ret, [this](MysqlConnection *c){
        this->releaseConn(c);
        });
}

void MysqlConnPool::releaseConn(MysqlConnection *c)
{
    m_setmutex.lock();
	assert(c != 0);
#if 0
    if(c != 0){
		::writelog("mysqlConnPool releaseConn error!");
		m_setmutex.release();
		return;
	}
#endif
    // 最多保存maxconn个连接
    if (m_connset.size() >= m_maxConn)
    {
        delete c;
    }
    else
    {
        m_connset.push_front(c);
    }
    m_setmutex.release();
}

