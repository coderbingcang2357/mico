#include <unistd.h>
#include "testresetpwd.h"
#include "../userpasswordrecover.h"
#include "mysqlcli/mysqlconnpool.h"
#include "../useroperator.h"
#include "../server.h"

void TestUserPasswordRecover::testVerfyCodeTimeout()
{
    printf("\n--TestUserPasswordRecover\n");
    MysqlConnPool pool;
    UserOperator uo(&pool);
    MicoServer ms;
    ms.init();
    UserPasswordRecover recover(&ms, &uo);

    recover.m_codeTimeout = 1000;
    recover.insertVerfycode(1, "Test", "12345");
    sleep(2);
    recover.insertVerfycode(1, "Test", "12345");
    recover.insertVerfycode(1, "Test", "12346");
    sleep(2);
}
