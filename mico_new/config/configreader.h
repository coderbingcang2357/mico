#ifndef CONFIG_READER_H
#define CONFIG_READER_H
#include <string>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <list>

class ServerConfig
{
public:
    std::string type;
    uint32_t id;
    std::string ip;
    uint16_t port;
    std::string property;
};

class Configure
{
private:
    void parse(const char *token, const char *value);
    static Configure config;

public:
    static Configure *getConfig();
	bool getSlaveNodePort(const std::string &str, std::string *port); 
    static int getConfigFilePath(char *buf, int sizeofbuf);

    Configure();
    int read(const char *path);
    bool readServers(std::ifstream *f);
    bool readLine(std::ifstream *f, std::string *line);
    bool readUncommentLine(std::ifstream *f, std::string *line);
    

    // 外部连接服务器 ExtConnSrv 端口
    in_port_t extconn_srv_port;
    // 外部连接服务器 ExtConnSrv 地址
    std::string extconn_srv_address;
    // 数据库服务器 DataBaseeSrv 地址
    std::string databasesrv_address;
    // 数据库服务器 DataBaseeSrv 端口
    in_port_t   databasesrv_port;
    // 数据库服务器连接的mysql地址
    int databasesrv_sql_port;
    std::string databasesrv_sql_host;
    std::string databasesrv_sql_user;
    //databasesrv_sql_pwd        = yill3813
    std::string databasesrv_sql_pwd;
    std::string databasesrv_sql_db;
    // 通知服务器连接的mysql地址
    int notifysrv_sql_port;
    std::string notifysrv_sql_host;
    std::string notifysrv_sql_user;
    std::string notifysrv_sql_pwd;
    std::string notifysrv_sql_db;
    std::string logfilepath;

    // redis服务器地址
    std::string redis_address;
    std::string redis_pwd;
    int redis_port;

    uint16_t file_redis_port;

    // shm cache memory size
    int shmOnlineCacheSize; // 最多允许多少用户登录, 服务器缓存的能保存的最大登录用户数

    // uint16_t localport;
    std::string localip;

    int serverid;

    std::string serverlistMysqlServer;

    std::string dbproxy_addr;
    uint16_t dbproxy_port;

    std::string reportmail;
    std::list<ServerConfig> servers;

    std::string masternodeip;
    std::string slavenodeip;
    std::string masternode_port;
    //std::string slavenode_port1;
    //std::string slavenode_port2;
    //std::string slavenode_port3;
    std::string rabbitmq_addr;
};


#endif
