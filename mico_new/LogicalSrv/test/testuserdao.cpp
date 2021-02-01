#include <assert.h>
#include "util/logwriter.h"
#include "./testuserdao.h"
#include "../dbaccess/user/userdao.h"

void TestUserDao::testGetClusterCount()
{
    ::writelog("\n\n--TestUserDao");
    UserDao ud;
    int allclus;
    int ownclus;
    int r = ud.getUserDeviceClusterCount(100000, &allclus, &ownclus);
    assert(r == 0);
    assert(allclus == 3);
    assert(ownclus == 1);

    // 两个都超过限制
    ud.m_maxclucount = 1;
    ud.m_maxjoinclucount = 3;
    assert(!ud.isClusterLimitValid(100000));
    // 可创建的群数超过限制, 可加入的总群数没有超过限制
    ud.m_maxclucount = 1;
    ud.m_maxjoinclucount = 4;
    assert(!ud.isClusterLimitValid(100000));
    // 可创建的群数没有超过限制, 可加入的总群数超过限制
    ud.m_maxclucount = 2;
    ud.m_maxjoinclucount = 3;
    assert(!ud.isClusterLimitValid(100000));
    // 两个都不超过限制
    ud.m_maxclucount = 2;
    ud.m_maxjoinclucount = 4;
    assert(ud.isClusterLimitValid(100000));
    ::writelog("ok");
}
