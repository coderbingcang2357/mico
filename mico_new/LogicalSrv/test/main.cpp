#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <iconv.h>
#include <locale>
#include <unordered_set>

//#include "../Config/Debug.h"
//#include "../Init/SystemInit.h"
//#include "../ShareMem/ShareMem.h"
#include "../Business/Business.h"
#include "../onlineinfo/onlineinfo.h"
#include "../onlineinfo/shmcache.h"
#include "util/logwriter.h"
#include "config/configreader.h"
#include "msgqueue/ipcmsgqueue.h"
#include "../tcpserver/tcpserver.h"
#include "../messageforward.h"
#include "../useroperator.h"
#include "mysqlcli/mysqlconnpool.h"
#include "mysqlcli/mysqlconnection.h"
#include "redisconnectionpool/redisconnectionpool.h"
#include "thread/threadwrap.h"
#include "../sessionkeygen.h"
#include "thread/mutexwrap.h"
#include "thread/mutexguard.h"
#include "../fileuploadredis.h"
#include "serverinfo/serverinfo.h"
#include "./testmessateprocessor.h"
#include "../sessionkeygen.h"
#include "./testgetclusterlist.h"
#include "./testgetclusteruserinfo.h"
#include "./testresetpwd.h"
#include "./testusermodifymail.h"
#include "./testscenedal.h"
#include "./testuserdao.h"
#include "./testclusterdal.h"
#include "./testdelscenelog.h"
#include "mysqlcli/mysqlconnpool.h"
#include "./testreadallonlinefromredis.h"

bool runasdaemon = false;
void testUserOperator();
//int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen);
//int u2g(char *inbuf,int inlen,char *outbuf,int outlen);
//int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen);
void getEncoding();
void testRedisPool();
void testGuid();
void testFileUploadRedis();
void testServerInfo();
void testSessionKey();

int main(int argc, char* argv[])
{
    //setlocale(LC_CTYPE, "en_US.UTF-8");


    //char configpath[1024];
    Configure *config = Configure::getConfig();
    config->databasesrv_sql_host="10.1.240.39";
    config->databasesrv_sql_port=3306;
    config->databasesrv_sql_user="ctdb";
    config->databasesrv_sql_db="test";
    config->databasesrv_sql_pwd="SSDD5512";

    config->redis_pwd = "3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb";
    config->redis_port = 6379;
    config->redis_address = "10.1.240.39";
    //Configure::getConfigFilePath(configpath, sizeof(configpath));

    //if (config->read(configpath) != 0)
    //{
    //    exit(-1);
    //}

    logSetPrefix("LogicalServer");

    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);
    //-------------------tes
    //getEncoding();
    //testUserOperator();
    //testRedisPool();
    //testGuid();
    //testFileUploadRedis();
    //testServerInfo();

    //testMessageQueue();
    //testmessgeProcessor(&server);
    //testGetUserInfo(&server);
    //testLoginReq(&server);
    //testLoginVerfy(&server);
    //testSessionKey();
    testGetClusterList();
    testGetClusterUserList();

    //TestUserPasswordRecover trpwd;
    //trpwd.testVerfyCodeTimeout();

    //TestUserModifyMail tmm;
    //tmm.testCodeTimeout();

    TestSceneDal tsd;
    tsd.testSceneDal();
    TestUserDao tud;
    tud.testGetClusterCount();

    TestClusterDal tcd;
    tcd.testLimit();

    TestDelSceneUploadLog tdellog;
    tdellog.test();
    return 0;
}

void getEncoding()
{
    MysqlConnection *sql =  MysqlConnPool::getConnection();
    if (sql->Select("show variables like 'chara%'") != 0)
    {
        printf("get enc error\n");
        return;
    }
    MYSQL_RES *r = sql->Res();
    for (uint32_t i = 0; i < r->row_count; ++i)
    {
        MYSQL_ROW row = sql->FetchRow(r);
        printf("%s ", row[0]);
        printf("%s \n", row[1]);
    }
}

void testUserOperator()
{
    MysqlConnPool pool;
    UserOperator u(&pool);
    DevClusterInfo info;
    if (u.getClusterInfo(0, uint64_t(144115188075955873), &info) < 0)
    {
        printf("get cluster info error");
    }
    else
    {
        printf("notename: %s\n", info.noteName.c_str());
    }
    char *p = (char *)info.noteName.c_str();
    printf("notename: %s\n", p);
    // notename 转成latin1
    //char out[100];
    //int l = u2g((char *)info.noteName.c_str(), info.noteName.size() + 1, out, 100);
    //printf("utf32: %ls\n", out);
    //wchar_t  *tmp = (wchar_t *)out;
    //printf("sizeof wchar %d\n", sizeof(wchar_t));
    //printf("wchar_t %ls\n", tmp);
}

//代码转换:从一种编码转为另一种编码
//int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)
//{
//    iconv_t cd;
//    int rc;
//    char **pin = &inbuf;
//    char **pout = &outbuf;
//
//    cd = iconv_open(to_charset,from_charset);
//    if (cd==0)
//        return -1;
//    memset(outbuf,0,outlen);
//    uint64_t inlentmp = inlen;
//    uint64_t outlentmp = outlen;
//    if (iconv(cd, pin, &inlentmp, pout, &outlentmp) == -1)
//    {
//        perror("iconv: ");
//        printf("%d, %d, %d\n", EILSEQ, EINVAL,errno);
//        return -1;
//    }
//    iconv_close(cd);
//    return 0;
//}
////UNICODE码转为GB2312码
//int u2g(char *inbuf,int inlen,char *outbuf,int outlen)
//{
//    return code_convert("utf-8", "utf-32",inbuf,inlen,outbuf,outlen);
//}
//
////GB2312码转为UNICODE码
//int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
//{
//    return code_convert("gb2312","utf-8",inbuf,inlen,outbuf,outlen);
//}

void testRedisPool_help(RedisConnectionPool *pool)
{
    std::shared_ptr<redisContext> rc = pool->get();
    std::shared_ptr<redisContext> rc2 = pool->get();
    assert(pool->connectionSize() == 0);
}

void testRedisPool()
{
    Configure *c = Configure::getConfig();
    RedisConnectionPool pool(c->redis_address, c->redis_port, c->redis_pwd);
    testRedisPool_help(&pool);
    assert(pool.connectionSize() == 2);
    printf("test ok.\n");
}

class TP : public Proc
{
public:
    TP(std::unordered_set<uint64_t> *s, MutexWrap *m, int *c) : set(s),mutex(m), cnt(c) {}
    void run()
    {
        for (int i = 0; i < 100 * 10000; ++i)
        {
            uint64_t r = genGUID();
            {
                MutexGuard g(mutex);
                auto it= set->find(r);
                if (it != set->end())
                {
                    cnt++;
                }
                set->insert(r);
            }
        }
    }

std::unordered_set<uint64_t> *set;
MutexWrap *mutex;
int *cnt;
};

void testGuid()
{
    std::unordered_set<uint64_t> set;
    int cnt = 0;
    {
    MutexWrap mutex;
    TP tp(&set,&mutex, &cnt);
    Thread t(&tp);
    Thread t2(&tp);
    Thread t3(&tp);
    }
    printf("set: %lu, cnt:%d\n",
        set.size(), cnt);
    assert(cnt == 0);

    //auto it = set.begin();
    //for (int c = 0; c < 30 && it != set.end(); ++c)
    //{
    //    printf("first:%lx\n", *it);
    //    ++it;
    //}
}

void testFileUploadRedis()
{
    FileUpAndDownloadRedis fr = FileUpAndDownloadRedis::getInstance("127.0.0.1", 6379, "");
    fr.insertUploadUrl("233", 233, "aaa.b", 233);
    fr.insertDownloadUrl("233", 233, "aaa.a");
}

void testServerInfo()
{
    bool r = ServerInfo::readServerInfo();
    assert(r);
    std::shared_ptr<ServerInfo> si = ServerInfo::getServerInfo(1);
    assert(si->ip() == "127.0.0.1");
    //assert(si->port() == 8885);
    //assert(si->type() == "logical");
    std::shared_ptr<ServerInfo> si2 = ServerInfo::getServerInfo(2);
    assert(si2->ip() == "127.0.0.1");
    //assert(si2->port() == 8000);
    //assert(si2->type() == "file");
    std::string fip = UploadServerInfo::getServrIP(2);
    assert(fip == "127.0.0.1");
    std::string ipport = UploadServerInfo::getServrUrl(2);
    assert(ipport == "127.0.0.1:8000");
}

void testSessionKey()
{
    char buf[16];
    ::genSessionKey(buf, sizeof(buf));
    printf("\n");
    for (uint32_t i = 0; i < sizeof(buf); ++i)
    {
        printf("%02x ", uint8_t(buf[i]));
    }
    printf("\n");
}
