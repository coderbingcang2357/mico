#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <string.h>
#include <list>
#include <vector>
#include <time.h>
#include <algorithm>
#include <malloc.h>
#include <curl/curl.h>

#include "../redisconnectionpool/redisconns.h"
#include "redisconnectionpool/redisconnectionpool.h"
#include "config/configreader.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "mysqlcli/mysqlconnection.h"
#include "mysqlcli/mysqlconnpool.h"
#include "util/util.h"

bool isRun = true;
void sigusr1(int)
{
    isRun = false;
}

class OnlineClient
{
public:
    union
    {
    uint64_t id = 0;
    uint64_t userid;
    uint64_t devid;
    };
    std::string name;
    std::string sockaddr;
    std::string mac;
    std::string logintime;
    uint16_t clientVersion = 0;
    uint8_t loginserverID;

};

class User
{
public:
    uint64_t ID;
    std::string account;
    std::string mail;
    std::string createDate;
    std::string lastLoginDate;
    int onlinetime; // 总在线时长
};

class Device
{
public:
    uint64_t id;
    std::string name;
    std::string mac;
    std::string lastloginDate;
    std::string createDate;
    int onlinetime; // 总在线时长
};
void getReport(std::string *report);
bool getOnlineClient(std::vector<OnlineClient> *oc);
bool readClient(OnlineClient *oc, char *str, uint32_t strlen);
bool getNewRegisterUser(std::list<User> *usr, MysqlConnection *conn);
bool getActiveDevice(std::list<Device> *dev, MysqlConnection *conn);
bool getActiveAccount(std::list<User> *usr, MysqlConnection *conn);
void userListToReportString(const std::list<User> &usr, std::string *outreport);
void deviceListToReportString(const std::list<Device> &dev, std::string *outreport);
void sendMail(const std::string &report);

// 当这样执行: micoreport_d -daemon > log时, printf会入到log中, 但是这个时候输出
// 是全缓冲的, 只有当缓冲区写满后或者进程退出后才会真正的写入到文件中
// siguser1 to exit
int main(int argc, char **argv)
{
    bool runasdaemon = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
        {
            runasdaemon = true;
        }
        else
        {
            printf("Logical Server: Unknow parameter: %s.\n", argv[i]);
        }
    }

    if (runasdaemon)
    {
        int err = daemon(1, // don't change dir
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::printf("Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    // siguser1 to exit
    sighandler_t re = signal(SIGUSR1, sigusr1);
    if (re == SIG_ERR)
    {
        ::printf("signal Failed: %s", strerror(errno));
        return 2;
    }

    // read config
    char configpath[1024];
    Configure *config = Configure::getConfig();
    Configure::getConfigFilePath(configpath, sizeof(configpath));

    if (config->read(configpath) != 0)
    {
        printf("read config failed: %s\n", configpath);
        return 3;
    }

    bool issend = false;//用来实现一天只发一次
    for (;isRun;)
    {
        sleep(1); // 1 sec
        time_t t = time(0);
        tm curt;
        localtime_r(&t, &curt);
        //static int cur = 0;
        //curt.tm_mday = cur++;
        //cur = cur % 30;
        // 每月13号, 28号
        if ((curt.tm_mday == 13 || curt.tm_mday == 13) && !issend) // week of day: range: 0-6
        {
            if (curt.tm_hour == 10) // 上午10点
            {
                printf("send, curday: %d\n", curt.tm_mday);
                std::string report;
                getReport(&report);
                sendMail(report);
                issend = true;
            }
        }
        else if (curt.tm_mday != 13 && curt.tm_mday != 13)
        {
            issend = false;
        }
    }

    return 0;
}

void getReport(std::string *report)
{
    std::vector<OnlineClient> ocl;
    getOnlineClient(&ocl);
    std::sort(ocl.begin(), ocl.end(),[](const OnlineClient &l, const OnlineClient &r){
        return l.id < r.id;
    });
    report->append("<h1>在线用户和设备(Mac地址为空的是用户)</h1>");
    if (ocl.size() > 0)
    {
        report->append("<table>\n"
                "<thead>"
                "<tr >"
                    "<th>ID</th>"
                    "<th>Name</th>"
                    "<th>sockaddr</th>"
                    "<th>mac</th>"
                    "<th>logintime</th>"
                    "<th>clientversion</th>"
                    "<th>loginserverID</th>"
                "</tr>"
                "</thead>\n"
                "<tbody>\n"
        );
        for (auto it = ocl.begin(); it != ocl.end(); ++it)
        {
           OnlineClient &oc = *it;
            char buf[1024];
            snprintf(buf, sizeof(buf),
                    "<tr style=\"background:lightgrey;\">"
                        "<td>%lu</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%u</td>"
                        "<td>%u</td>"
                    "</tr>\n",
                    oc.id,
                    oc.name.c_str(),
                    oc.sockaddr.c_str(),
                    oc.mac.c_str(),
                    oc.logintime.c_str(),
                    oc.clientVersion,
                    oc.loginserverID);
           report->append(buf);
        }
        report->append("</tbody>\n");
        report->append("</table>\n");
    }
    else
    {
        report->append("Empty");
    }

    // get mysql conn
    MysqlConnection conn;
    Configure *cfg = Configure::getConfig();
    SqlParam p;
    p.host = cfg->databasesrv_sql_host;
    p.port = cfg->databasesrv_sql_port;
    p.user = cfg->databasesrv_sql_user;
    p.passwd = cfg->databasesrv_sql_pwd;
    p.db = cfg->databasesrv_sql_db;
    if (!conn.connect(p))
    {
        printf("conn to mysql failed");
        return;
    }

    // 新注册用户
    std::list<User> usr;
    getNewRegisterUser(&usr, &conn);
    std::string newUserReportstr;
    report->append("<h1>15天新注册用户</h1>");
    userListToReportString(usr, &newUserReportstr);
    report->append(newUserReportstr);

    // 15天活跃用户
    std::list<User> activeUserIn7Days;
    ::getActiveAccount(&activeUserIn7Days, &conn);
    std::string activeUserreportstr;
    report->append("<h1>15天活跃的用户</h1>");
    userListToReportString(activeUserIn7Days, &activeUserreportstr);
    report->append(activeUserreportstr);

    // 15天活跃设备
    std::list<Device> activeDeviceIn7Days;;
    ::getActiveDevice(&activeDeviceIn7Days, &conn);
    std::string activeDevReport;
    report->append("<h1>15天活跃的设备</h1>");
    ::deviceListToReportString(activeDeviceIn7Days, &activeDevReport);
    report->append(activeDevReport);

    report->append("<br/>");
    report->append("<br/>");
    report->append("<br/>");
}


bool getOnlineClient(std::vector<OnlineClient> *ocl)
{
    Configure *cfg = Configure::getConfig();
    std::shared_ptr<redisContext> c = RedisConn::getRedisConn(
        cfg->redis_address,
        cfg->redis_port,
        cfg->redis_pwd);
    if (!c)
    {
        printf("get redis conn failed.\n");
        return false;
    }
    // find in redis
    redisReply *reply;
    reply = (redisReply *)redisCommand(c.get(),
                             "hgetall online_info");

    bool iserr = RedisConnectionPool::hasError(c.get(), reply);

    if (!iserr)
    {
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (uint32_t i = 1; i < reply->elements; i+=2)
            {
                redisReply *r = reply->element[i];
                if (r->type == REDIS_REPLY_INTEGER)
                {
                    printf("int: %lld\n", r->integer);
                }
                else if (r->type == REDIS_REPLY_STRING)
                {
                    OnlineClient oc;
                    readClient(&oc, r->str, r->len);
                    ocl->push_back(oc);
                }
                else
                {
                    printf("error replay type: %d\n", r->type);
                }
            }
        }
        else
        {
            printf("error replay type noarray: %d\n", reply->type);
            return false;
        }
    }
    if (reply != 0)
        freeReplyObject(reply);

    return iserr;
}

bool readClient(OnlineClient *oc, char *str, uint32_t strlen)
{
    rapidjson::Document doc;
    std::string json(str, strlen);
    doc.Parse(json.c_str());
    assert(doc.IsObject());

    if (doc.HasMember("userid"))
    {
        oc->userid = doc["userid"].GetUint64();

        if (doc.HasMember("lastHeartBeatTime"))
        {
            oc->logintime = doc["lastHeartBeatTime"].GetString();
        }

    }

    if (doc.HasMember("devid"))
    {
        oc->devid = doc["devid"].GetUint64();
        if (doc.HasMember("lastHeartBeatTime"))
        {
            time_t t = doc["lastHeartBeatTime"].GetUint64();
            char buf[1024];
            buf[0] = 0;
            ctime_r(&t, buf);
            oc->logintime = buf;
        }
    }

    if (doc.HasMember("account"))
    {
        const rapidjson::Value &v = doc["account"];
        oc->name = std::string(v.GetString(), v.GetStringLength());
    }


    if (doc.HasMember("name"))
    {
        oc->name = doc["name"].GetString();
    }

    if (doc.HasMember("clientversion"))
    {
        oc->clientVersion = doc["clientversion"].GetUint();
    }


    if (doc.HasMember("sockaddr"))
    {
        oc->sockaddr = doc["sockaddr"].GetString();
    }

    if (doc.HasMember("address"))
    {
        oc->sockaddr = doc["address"].GetString();
    }

    if (doc.HasMember("macAddr"))
    {
        oc->mac = doc["macAddr"].GetString();
    }

    if (doc.HasMember("loginserverid"))
    {
        oc->loginserverID = doc["loginserverid"].GetInt();
    }

    return true;
}

bool getNewRegisterUser(std::list<User> *usr, MysqlConnection *conn)
{
    static uint64_t lastUserID = 0;
    std::string query;
    if (lastUserID == 0)
    {
        query = "select ID, account, mail, createDate , lastloginTime, onlinetime from T_User where createDate > date_add(now(), interval -15 day)";
    }
    else
    {
        query = std::string("select ID, account, mail, createDate, lastloginTime, onlinetime from T_User where id> ")
                + std::to_string(lastUserID - 1) + " order by ID asc";
    }

    ;
    int r = conn->Select(query);
    if (r < 0)
    {
        return false;
    }
    auto res = conn->result();
    int rc = res->rowCount();
    //ID, account, mail, createDate
    for (auto i = 0; i < rc; ++i)
    {
        User u;
        auto row = res->nextRow();
        u.ID = row->getUint64(0);
        if (lastUserID < u.ID)
        {
            lastUserID = u.ID;
        }
        u.account = row->getString(1);
        u.mail = row->getString(2);
        u.createDate = row->getString(3);
        u.lastLoginDate = row->getString(4);
        u.onlinetime = row->getInt32(5);
        usr->push_back(u);
    }
    return true;
}

bool getActiveDevice(std::list<Device> *dev, MysqlConnection *conn)
{
    assert(conn != 0);
    const char *query = "select "
                 " ID, name, macAddr, createDate, lastOnlineTime, onlinetime "
                 " from T_Device where lastOnlineTime > date_add(now(), interval - 15 day)";
    int r = conn->Select(query);
    if (r < 0)
    {
        return false;
    }
    auto res = conn->result();
    int rc = res->rowCount();
    //ID, account, mail, createDate
    for (auto i = 0; i < rc; ++i)
    {
        Device d;
        auto row = res->nextRow();
        d.id= row->getUint64(0);
        d.name = row->getString(1);
        char mac[6];
        ::uint64ToMacAddr(row->getUint64(2), mac);
        d.mac = ::byteArrayToHex((uint8_t *)mac, 6);
        d.createDate = row->getString(3);
        d.lastloginDate = row->getString(4);
        d.onlinetime = row->getInt32(5);
        dev->push_back(d);
    }
    return true;
}

bool getActiveAccount(std::list<User> *usr, MysqlConnection *conn)
{
    assert(conn != 0);
    const char *query = " select ID, account, mail, createDate, lastloginTime, onlinetime "
                        " from T_User "
                        " where lastloginTime > date_add(now(), interval -15 day)";
    int r = conn->Select(query);
    if (r < 0)
    {
        return false;
    }
    auto res = conn->result();
    int rc = res->rowCount();
    //ID, account, mail, createDate
    for (auto i = 0; i < rc; ++i)
    {
        User u;
        auto row = res->nextRow();
        u.ID = row->getUint64(0);
        u.account = row->getString(1);
        u.mail = row->getString(2);
        u.createDate = row->getString(3);
        u.lastLoginDate = row->getString(4);
        u.onlinetime = row->getInt32(5);
        usr->push_back(u);
    }
    return true;
}

void userListToReportString(const std::list<User> &usr, std::string *report)
{
    if(usr.size() > 0)
    {
        // ID, account, mail, createDate
        report->append("<table>\n"
                "<thead>"
                "<tr >"
                    "<th>ID</th>"
                    "<th>Account</th>"
                    "<th>mail</th>"
                    "<th>createDate</th>"
                    "<th>最后登录时间</th>"
                    "<th>总在线时长</th>"
                "</tr>"
                "</thead>\n"
                "<tbody>\n");
        for (auto &u : usr)
        {
            char buf[1024];
            snprintf(buf, sizeof(buf),
                    "<tr style=\"background:lightgrey;\">"
                        "<td>%lu</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%d分, %d秒</td>"
                    "</tr>\n", 
                    u.ID,
                    u.account.c_str(),
                    u.mail.c_str(),
                    u.createDate.c_str(),
                    u.lastLoginDate.c_str(),
                    u.onlinetime/60,
                    u.onlinetime%60);
            report->append(buf);
        }
        report->append("</tbody>\n");
        report->append("</table>\n");
    }
    else
    {
        report->append("<p>empty</p>");
    }
}

void deviceListToReportString(const std::list<Device> &dev,
                            std::string *report)
{
    if(dev.size() > 0)
    {
        // ID, account, mail, createDate
        report->append("<table>\n"
                "<thead>"
                "<tr >"
                    "<th>ID</th>"
                    "<th>name</th>"
                    "<th>MAC</th>"
                    "<th>createDate</th>"
                    "<th>最后登录时间</th>"
                    "<th>总在线时长</th>"
                "</tr>"
                "</thead>\n"
                "<tbody>\n");
        for (auto &d : dev)
        {
            char buf[1024];
            snprintf(buf, sizeof(buf),
                    "<tr style=\"background:lightgrey;\">"
                        "<td>%lu</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%s</td>"
                        "<td>%d分, %d秒</td>"
                    "</tr>\n", 
                    d.id,
                    d.name.c_str(),
                    d.mac.c_str(),
                    d.createDate.c_str(),
                    d.lastloginDate.c_str(),
                    d.onlinetime/60,
                    d.onlinetime%60);
            report->append(buf);
        }
        report->append("</tbody>\n");
        report->append("</table>\n");
    }
    else
    {
        report->append("<p>empty</p>");
    }
}
//-----------------------------------------------------------------------------

struct RetString{ char *ptr;size_t len;};
void init_string(RetString *rs)
{
    rs->len = 0;
    rs->ptr = (char*) malloc(rs->len + 1);
    if (rs->ptr == NULL){
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    rs->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, RetString *s)
{
   size_t new_len = s->len + size*nmemb;
   s->ptr = (char*) realloc(s->ptr, new_len+1);
   if (s->ptr == NULL){
          fprintf(stderr, "realloc() failed\n");
          exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;
    return size*nmemb;
}

void sendMail(const std::string &report)
{
    CURL *curl;
    CURLcode res;

    // 返回信息
    RetString ret_str;
    init_string (&ret_str);

    curl_global_init (CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt (curl, CURLOPT_URL, "https://sendcloud.sohu.com/webapi/mail.send.json");
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        // time out 30 s
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        /*转义*/       
        char *html = curl_easy_escape (curl, report.c_str(), 0);

        Configure *c = Configure::getConfig();
        std::string postFileds = "api_user=";
        postFileds += "postmaster@co-trust.sendcloud.org";
        postFileds += "&api_key=";
        postFileds += "BtLX3ZgjIYaN3eFp";
        postFileds += "&from=";
        postFileds += "noreply@itmg.co-trust.com";
        postFileds += "&to=";
        postFileds += c->reportmail;
        postFileds += "&fromname=";
        postFileds += "Mico Server";
        postFileds += "&subject=";
        postFileds += "mico server";
        postFileds += "&html=";
        postFileds += html;

        //std::cout << postFileds << std::endl;

        curl_easy_setopt (curl, CURLOPT_POSTFIELDS, postFileds.c_str());

        // 获取返回
        curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, &ret_str);

        res = curl_easy_perform (curl);
        if (res != CURLE_OK)
            fprintf (stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror (res));
        else
            printf ("%s\n", ret_str.ptr);

        free (ret_str.ptr);
        curl_free(html);

        curl_easy_cleanup (curl);
    }

    curl_global_cleanup();
}

