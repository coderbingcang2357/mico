#ifndef DB_OPERATION_H
#define DB_OPERATION_H

#include <stdint.h>
#include <vector>

#include "mysqlcli/mysqlconnection.h"
#include "config/dbconfig.h"
#include "config/shmconfig.h"
//#include "databaseConnectionInfo.h"


class Notification;

class IDBoperator
{
public:
    virtual ~IDBoperator(){}
    virtual bool SaveNotification(const Notification& notification, uint64_t *insertid) = 0;
    virtual bool GetNotification(uint64_t objectID, std::vector<Notification *> *vNotification) = 0;
    virtual bool DeleteNotification(uint64_t objectID, uint64_t notifNumber) = 0;
	virtual bool DelOfflineMsgIfMoreThan300(uint64_t objectID, uint32_t maxnum) = 0;
};

class DBOperator : public IDBoperator
{
public:
    DBOperator();
    ~DBOperator();

    bool SaveNotification(const Notification& notification, uint64_t *insertid);
    bool GetNotification(uint64_t objectID, std::vector<Notification *> *vNotification);
    bool DeleteNotification(uint64_t objectID, uint64_t notifNumber);
	bool DelOfflineMsgIfMoreThan300(uint64_t objectID, uint32_t maxnum);

private:
    //void SetSqlPara(const DataBaseConnectionInfo& sqlPara) { m_sqlParam = sqlPara; }

private:
    MysqlConnection m_sql;
    //DataBaseConnectionInfo m_sqlParam;
};

#endif
