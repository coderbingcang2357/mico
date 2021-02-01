
#include "mysqlcli/mysqlconnection.h"
#include "util/util.h"
#include "timer/TimeElapsed.h"

void test();
void testSelectFromUser();
void testSelectFromUserCluster();
SqlParam *createSqlPara();

char *ip;
char *user;
char *pwd;
char *db;
int main(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("useage: %s ip user pwd dbname\h", argv[0]);
        return 0;
    }

    ip = argv[1];
    user = argv[2];
    pwd = argv[3];
    db = argv[4];

    test();
    return 0;
}

void test()
{
    testSelectFromUser();
    //testSelectFromUserCluster();
}

void testSelectFromUser()
{
    MysqlConnection conn;
    SqlParam *p = createSqlPara();
    conn.connect(p);

    TimeElap elap;
    elap.start();
    for (int i = 0; i < 10; ++i)
    {

        int r = conn.Select("select ID from T_User where account='wqs1'");
        assert(r >= 0);
        printf("get:\n");
        int c=getchar();
    }
    uint64_t timeused = elap.elaps();
    printf("timeuserd: %lus, %luus\n", timeused / (1000 * 1000), timeused % (1000 * 1000));
}

void testSelectFromUserCluster()
{
    MysqlConnection conn;
    SqlParam *p = createSqlPara();
    conn.connect(p);

    TimeElap elap;
    elap.start();
    for (int i = 0; i < 10000; ++i)
    {
        std::string stat("select deviceClusterID from T_User_DevCluster where userID=");
        stat.append(std::to_string(100039 + i));
        int r = conn.Select(stat);
        for(my_ulonglong i = 0; i != conn.Res()->row_count; i++)
        {
            uint64_t deviceClusterID;
            MYSQL_ROW row = conn.FetchRow(conn.Res());
            Str2Uint(row[0], deviceClusterID);
        }
        assert(r >= 0);
    }
    uint64_t timeused = elap.elaps();
    printf("timeuserd: %lus, %luus\n", timeused / (1000 * 1000), timeused % (1000 * 1000));

}

SqlParam *createSqlPara()
{
    SqlParam *p = new SqlParam;
    p->port = 3306;
    p->host = ip;
    p->db = db;
    p->user = user;
    p->passwd = pwd;
    return p;
}
