#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "configreader.h"
#include "util/util.h"

Configure::Configure(){} 
#if 0
	extconn_srv_port(8888),
    databasesrv_port(8887),
    logfilepath("/var/log/mico/log"),
    redis_address("127.0.0.1"),
    redis_port(6379),
	redis_pwd("3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb"),
    file_redis_port(6379),
    shmOnlineCacheSize(100 * 1024 * 1024), // 100 M
    // localport(9886),
    //localip("121.196.37.50"),
    //localip("127.0.0.1"),
    serverid(4),
    //serverid(1),
    serverlistMysqlServer("127.0.0.1:3306:root:SSDD5512:CTCloud:serverlist"),
    dbproxy_addr("127.0.0.1"),
    dbproxy_port(8989),
    reportmail("liushenghong@co-trust.com")
#endif

Configure Configure::config;

Configure *Configure::getConfig()
{
    return &config;
}
#if 0
bool Configure::getSlaveNodePort(const std::string &str, std::string *port) {

	if("slavenode_port1" == str){

		*port = slavenode_port1; 
	}   
	else if("slavenode_port2" == str) {

		*port = slavenode_port2;
	}   
	else if("slavenode_port3" == str) {

		*port = slavenode_port3;
	}   
	else{

		printf("getSlaveNodePort failed");
		return false;
	}   
	return true;
}
#endif

bool Configure::readLine(std::ifstream *f, std::string *line)
{
    if (!f->good())
        return false;

    std::getline(*f, *line);
    return true;
}

bool Configure::readUncommentLine(std::ifstream *f, std::string *line)
{
    for (;;)
    {
        if (!this->readLine(f, line))
            return false;
        uint32_t i = 0;
        std::string &refline = *line;
        for (; i < refline.size(); ++i)
        {
            if (!::isspace(refline[i]))
                break;
        }
        // an line with all char is space
        if (i == refline.size())
        {
            line->clear();
            continue;
        }

        else if (refline[i] == '#') // a comment line
        {
            continue;
        }

        else // ok a vaid line
        {
            if (i != 0)
            {
                *line = refline.substr(i);
            }
            return true;
        }
    }
}

int Configure::read(const char *path)
{
    printf("%s\n", path);
    std::ifstream f(path);
    if (!f.is_open())
    {
        printf("Config read failed");
        return -1;
    }

    std::string line;
    while (f.good())
    {
        if (!this->readUncommentLine(&f, &line))
            break;
    
        std::vector<string> token;
        splitString(line, " ", &token);
        if (token.size() == 2)
        {
            parse(token[0].c_str(), token[1].c_str());
        }
        else if (token.size() == 1)
        {
            if (token[0] == "[servers]")
            {
                if (!this->readServers(&f))
                    break;
            }
            else
            {
                printf("error config1 : %s\n", token[0].c_str());
                break;
            }
        }
        else
        {
            //
            printf("error config2 : %s\n", token[0].c_str());
            break;
        }
    }

    return 0;
}

void Configure::parse(const char *token, const char *value)
{
    if (strcasecmp(token, "extconn_srv_port") == 0)
    {
        this->extconn_srv_port = atoi(value);
    }
    else if (strcasecmp(token, "extconn_srv_address") == 0)
    {
        this->extconn_srv_address = std::string(value);
    }
    else if (strcasecmp(token, "databasesrv_address") == 0)
    {
        this->databasesrv_address = std::string(value);
    }
    else if (strcasecmp(token, "databasesrv_port") == 0)
    {
        this->databasesrv_port = atoi(value);
    }
    else if (strcasecmp(token, "databasesrv_sql_port") == 0)
    {
        this->databasesrv_sql_port = atoi(value);
    }
    else if (strcasecmp(token, "databasesrv_sql_host") == 0)
    {
        this->databasesrv_sql_host = std::string(value);
    }
    else if (strcasecmp(token, "databasesrv_sql_user") == 0)
    {
        this->databasesrv_sql_user = std::string(value);
    }
    else if (strcasecmp(token, "databasesrv_sql_pwd") == 0)
    {
        this->databasesrv_sql_pwd = std::string(value);
    }
    else if (strcasecmp(token, "databasesrv_sql_db") == 0)
    {
        this->databasesrv_sql_db = std::string(value);
    }
    else if (strcasecmp(token, "notifysrv_sql_port") == 0)
    {
        this->notifysrv_sql_port = atoi(value);
    }
    else if (strcasecmp(token, "notifysrv_sql_host") == 0)
    {
        this->notifysrv_sql_host = std::string(value);
    }
    else if (strcasecmp(token, "notifysrv_sql_user") == 0)
    {
        this->notifysrv_sql_user = std::string(value);
    }
    else if (strcasecmp(token, "notifysrv_sql_pwd") == 0)
    {
        this->notifysrv_sql_pwd = std::string(value);
    }
    else if (strcasecmp(token, "notifysrv_sql_db") == 0)
    {
        this->notifysrv_sql_db = std::string(value);
    }
    else if (strcasecmp(token, "logfile") == 0)
    {
        this->logfilepath = std::string(value);
    }
    else if (strcasecmp(token, "redis_address") == 0)
    {
        this->redis_address = std::string(value);
    }
    else if (strcasecmp(token, "redis_pwd") == 0)
    {
        this->redis_pwd = std::string(value);
    }
    else if (strcasecmp(token, "redis_port") == 0)
    {
        this->redis_port = atoi(value);
    }
    else if (strcasecmp(token, "file_redis_port") == 0)
    {
        this->file_redis_port = atoi(value);
    }
    else if (strcasecmp(token, "shm_online_cache_size") == 0)
    {
        this->shmOnlineCacheSize = atoi(value);
    }
    else if (strcasecmp(token, "localip") == 0)
    {
        this->localip = std::string(value);
    }
    else if (strcasecmp(token, "localport") == 0)
    {
        //this->localport = atoi(value);
    }
    else if (strcasecmp(token, "serverid") == 0)
    {
        this->serverid = atoi(value);
    }
    else if (strcasecmp(token, "serverlist_mysql_server") == 0)
    {
        this->serverlistMysqlServer = std::string(value);
    }
    else if (strcasecmp(token, "dbproxyserver") == 0)
    {
        char *pos = strchr((char *)value, ':');
        if (pos != 0)
        {
            *pos = 0;
            dbproxy_addr = std::string(value);
            dbproxy_port = atoi(pos+1);
        }
        else
        {
            printf("error config value: %s:%s\n", token, value);
        }
    }
    else if (strcasecmp(token, "reportmail") == 0)
    {
        this->reportmail = value;
    }
    else if (strcasecmp(token, "rabbitmq_addr") == 0)
    {
        this->rabbitmq_addr = value;
    }
    else if (strcasecmp(token, "masternodeip") == 0)
    {
        this->masternodeip = value;
    }
    else if (strcasecmp(token, "slavenodeip") == 0)
    {
        this->slavenodeip = value;
    }
    else if (strcasecmp(token, "masternode_port") == 0)
    {
        this->masternode_port = value;
    }
    //else if (strcasecmp(token, "slavenode_port1") == 0)
    //{
    //    this->slavenode_port1 = value;
    //}
    //else if (strcasecmp(token, "slavenode_port2") == 0)
    //{
    //    this->slavenode_port2 = value;
    //}
    //else if (strcasecmp(token, "slavenode_port3") == 0)
    //{
    //    this->slavenode_port3 = value;
    //}
    else
    {
        printf("Unknow config: %s value: %s\n", token, value);
        assert(false);
    }
}

bool Configure::readServers(std::ifstream *f)
{
    std::string line;
    bool hasend = false;
    while (this->readUncommentLine(f, &line))
    {
        if (line == "[/servers]")
        {
            hasend = true;
            break;
        }
        std::vector<std::string> tok;
        splitString(line, " ", &tok);
        // type id ip port prop
        if (tok.size() >= 4)
        {
            ServerConfig sc;
            sc.type = tok[0];
            sc.id = std::stoul(tok[1]);
            sc.ip = tok[2];
            sc.port = std::stoul(tok[3]);
            if (tok.size() == 5)
                sc.property = tok[4];
            this->servers.push_back(sc);
        }
        else
        {
            printf("Unknow config: %s\n", line.c_str());
            return false;
        }
    }
    if (!hasend)
    {
        printf("server config error");
    }
    return hasend;
}

// 1. 应用程序目录下面的mico.conf
// 2. 用户主目录下的 $HOME/mico.conf
// 3. /etc/mico/mico.conf
int Configure::getConfigFilePath(char *buf, int sizeofbuf)
{
    (void)sizeofbuf;
    char exepathbuf[1024];
    char *pdirofexe = 0;
    int n;
    if ((n = readlink("/proc/self/exe", exepathbuf, sizeof(exepathbuf))) == -1)
    {
        perror("readlink failed.");
        return 1;
    }
    exepathbuf[n] = 0;

    while (n >= 0 && exepathbuf[n] != '/')
    {
        n--;
    }
    if (n < 0)
    {
        pdirofexe = 0;
    }
    else
    {
        exepathbuf[n] = 0;
        pdirofexe = exepathbuf;
    }

    // 应用程序目录下面的mico.conf
    strcat(pdirofexe, "/mico.conf");
    // 存在并且可读
    if (access(pdirofexe, F_OK | R_OK) == 0) // ok
    {
        strcpy(buf, pdirofexe);
        return 0;
    }
    else
    {
        return -1;
    }

    // HOME 目录下的mico.conf
    char *home = exepathbuf;
    strcpy(home, getenv("HOME"));
    int len = strlen(home);
    if (home[len - 1] == '/')
    {
        home[len - 1] = 0;
        len--;
    }

    strcat(home, "/mico.conf");
    // 存在并且可读
    if (access(home, F_OK | R_OK) == 0) // ok
    {
        strcpy(buf, pdirofexe);
        return 0;
    }

    // /etc/mico/mico.conf
    if (access("/etc/mico/mico.conf", F_OK | R_OK) == 0)
    {
        strcpy(buf, "/etc/mico/mico.conf");
        return 0;
    }

    return -1;
}

