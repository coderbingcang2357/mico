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
#include <map>
#include <set>

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

class Device
{
public:
    Device() : id(0), mac(0), firm(0) {}
    Device(uint64_t id_, uint64_t mac_, uint64_t firm_)
        : id(id_), mac(mac_), firm(firm_){}
    uint64_t id;
    uint64_t mac;
    uint64_t firm;
};
void getReport();
bool readDevice(std::map<uint64_t, std::shared_ptr<std::list<Device>>> *dev, MysqlConnection *conn);
void changeFirmIfRepeat(std::shared_ptr<std::list<Device>> dev, std::list<Device> *out);
void updateFirm(const Device &d,  MysqlConnection *conn);
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

    getReport();
    return 0;
}

void getReport()
{
    // get mysql conn
    MysqlConnection conn;
    Configure *cfg = Configure::getConfig();
    SqlParam *p = new SqlParam;
    p->host = cfg->databasesrv_sql_host;
    p->port = cfg->databasesrv_sql_port;
    p->user = cfg->databasesrv_sql_user;
    p->passwd = cfg->databasesrv_sql_pwd;
    p->db = cfg->databasesrv_sql_db;
    if (!conn.connect(p))
    {
        printf("conn to mysql failed");
        return;
    }
    std::map<uint64_t, std::shared_ptr<std::list<Device>>> dev;
    std::list<Device> devshouldupdatefirm;
    readDevice(&dev, &conn);
    for (auto v : dev)
    {
        if (v.second->size() > 1)
        {
            changeFirmIfRepeat(v.second, &devshouldupdatefirm);
        }
    }
    for (auto &d : devshouldupdatefirm)
    {
        updateFirm(d, &conn);
        //printf("%lu, %lu, %lu\n", d.id, d.mac, d.firm);
    }
}

bool readDevice(std::map<uint64_t, std::shared_ptr<std::list<Device>>> *dev, MysqlConnection *conn)
{
    assert(conn != 0);
    const char *query = "select "
                 " ID, macAddr, firmClusterID"
                 " from T_Device";
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
        d.mac = row->getUint64(1);
        d.firm = row->getUint64(2);
        //dev->push_back(d);
        //dev->insert(std::make_pair(d.mac, d));
        auto it = dev->find(d.mac);
        if (it != dev->end())
        {
            it->second->push_back(d);
        }
        else
        {
            std::list<Device> *l = new std::list<Device>();
            l->push_back(d);
            dev->insert(std::make_pair(d.mac, std::shared_ptr<std::list<Device>>(l)));
        }
    }
    return true;
}

void changeFirmIfRepeat(std::shared_ptr<std::list<Device>> dev, std::list<Device> *out)
{
    std::set<uint64_t> firm;
    int startfirm = 100;
    for (auto &d : *dev)
    {
        auto fit = firm.find(d.firm);
        if (fit != firm.end())
        {
            // find a valid firm
            for (;;++startfirm)
            {
                auto newit = firm.find(startfirm);
                // ok
                if (newit == firm.end())
                {
                    firm.insert(startfirm);
                    out->push_back(Device(d.id, d.mac, startfirm));
                    ++startfirm;
                    break;
                }
                else
                {
                    continue;
                }
            }
        }
        else
        {
            firm.insert(d.firm);
        }
    }
}

void updateFirm(const Device &d,  MysqlConnection *conn)
{
    assert(conn != 0);
    char query[1024];
    snprintf(query, sizeof(query),
        "update T_Device set firmClusterID=%lu where ID=%lu",
        d.firm, 
        d.id);
    int r = conn->Update(query);
    if (r < 0)
    {
        printf("update firm failed:id:%lu, firm:%lu\n", d.id, d.firm);
    }
}
