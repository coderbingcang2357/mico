#include <memory>
#include <functional>
#include "userdao.h"
#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "util/logwriter.h"

UserDao::UserDao(): m_maxjoinclucount(500)
//UserDao::UserDao(): m_maxclucount(10), m_maxjoinclucount(10)
{
}

int UserDao::getPasswordAndID(const std::string &account,
                    uint64_t *id,
                    std::string *pwd)
{
    return password.getPasswordAndID(account, id, pwd);
}

int UserDao::modifyPassword(const std::string &account, uint64_t id, const std::string &newpwd)
{
    return password.modifyPassword(account, id, newpwd);
}

// 最多可以加入多少个群(包令自个创建的)
int UserDao::maxJoinDeviceClusterCount()
{
    return m_maxjoinclucount;
}

// allclus:取用户加入了多少个群(包含自己创建的)
// ownclus:用户创建了多少个群
int UserDao::getUserDeviceClusterCount(uint64_t userid, int *allclus)
{
    std::unique_ptr<MysqlConnection, std::function<void(MysqlConnection *)> >
    sql(MysqlConnPool::getConnection(), [](MysqlConnection *p) {
            MysqlConnPool::release(p);
        });
    if(!sql)
        return -1;
    char query[1024];
    //snprintf(query, sizeof(query), "select * from "
    //        "(select count(*) as allclu from  T_User_DevCluster where userid=%lu) as alal, "
    //        "(select count(*) as ownclu from T_User_DevCluster where userid=%lu and role =1) as b",
    //        userid, userid);
    snprintf(query, sizeof(query),
            "select count(*) from  T_User_DevCluster where userid=%lu",
            userid);
    int r = sql->Select(std::string(query));
    if (r == 0)
    {
        MYSQL_ROW row = sql->FetchRow(sql->Res());
        *allclus = atoi(row[0]);
        //*ownclus = atoi(row[1]);
        return 0;
    }
    else
    {
        return -1;
    }
}

// 用户群是否达到限制
bool UserDao::isClusterLimitValid(uint64_t userid)
{
    int allclus;
    int r = getUserDeviceClusterCount(userid, &allclus);
    if (r == 0
        && allclus < maxJoinDeviceClusterCount())
    {
        return true;
    }
    else 
    {
        ::writelog(InfoType, "create cluster limit: %d", allclus);
        return false;
    }
}
