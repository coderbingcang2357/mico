#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "../configreader.h"

void test()
{
    Configure conf;
    char path[1024];
    strcpy(path, getenv("HOME"));
    strcat(path, "/mico.conf");
    int r = conf.read(path);
    assert(r == 0);
    assert(conf.extconn_srv_port = 8888);
    assert(conf.extconn_srv_address == std::string("127.0.0.1"));
    assert(conf.databasesrv_address == std::string("127.0.0.1"));
    assert(conf.databasesrv_port == 8887);

    assert(conf.databasesrv_sql_port == 3306);
    assert(conf.databasesrv_sql_host == 
        std::string("localhost"));
    assert(conf.databasesrv_sql_user == 
        std::string("root"));
    assert(conf.databasesrv_sql_pwd == 
        std::string("SSDD5512"));
    assert(conf.databasesrv_sql_db == 
        std::string("CTCloud"));

    assert(conf.notifysrv_sql_port == 3306);
    assert(conf.notifysrv_sql_host == 
        std::string("localhost"));
    assert(conf.notifysrv_sql_user == 
        std::string("root"));
    assert(conf.notifysrv_sql_pwd ==
        std::string("vfgf#h89"));
    assert(conf.notifysrv_sql_db == 
        std::string("CTCloud"));

    printf("OK\n");
}

void test2()
{
    char path[1024];
    int res = Configure::getConfigFilePath(path, sizeof(path));
    printf("%s\n", path);
    assert(res == 0);
    printf("OK\n");
}

int main()
{
    test();
    test2();
    return 0;
}
