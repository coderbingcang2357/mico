#include "../useroperator.h"
#include "mysqlcli/mysqlconnpool.h"
#include <iostream>

void testGetClusterList()
{
    MysqlConnPool pool;
    UserOperator uo(&pool);
    uint64_t userid=100000;
    std::vector<DevClusterInfo> clu;
    uo.getClusterListOfUser(userid, &clu);
    for (auto &c : clu)
    {
        std::cout << c.id << " |"
                << c.account << " |"
                << c.noteName << " |"
                << uint32_t(c.role) <<" |"
                << c.description << std::endl;
    }
}
