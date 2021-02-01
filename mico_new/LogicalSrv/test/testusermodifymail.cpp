#include <unistd.h>
#include "testusermodifymail.h"
#include "../usermodifymail.h"
#include "../server.h"
#include "../useroperator.h"
#include "mysqlcli/mysqlconnpool.h"

void TestUserModifyMail::testCodeTimeout()
{
    printf("\n--TestUserModifyMail\n");

    MysqlConnPool p;
    UserOperator op(&p);
    MicoServer ms;
    UserModifyMail umm(&ms, &op);
    umm.m_codetimeout = 1;
    umm.insertVerfyCode(1, "234", "1");
    sleep(2);
    umm.insertVerfyCode(1, "235", "1");
    umm.insertVerfyCode(1, "236", "1");
    sleep(2);
}
