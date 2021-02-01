#include "./testgetclusteruserinfo.h"
#include "../useroperator.h"
#include "mysqlcli/mysqlconnpool.h"
#include <iostream>

void testGetClusterUserList()
{
    std::cout << "\n--Test get userinfo list in cluster\n";
    MysqlConnPool pool;
    UserOperator uo(&pool);
    std::vector<UserMemberInfo> inf;
    uint64_t cluid=144115188075956146ll;
    uo.getUserMemberInfoList(cluid, &inf);
    for (auto &v : inf)
    {
        std::cout << v.id << " "
                << v.account << " "
                << uint32_t(v.role) << " "
                << uint32_t(v.head) << " " << std::endl;
    }
}
