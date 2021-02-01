#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "connectionserver.h"
#include "util/logwriter.h"
#include "config/configreader.h"
#include "serverinfo/serverinfo.h"

int main(int argc, char **argv)
{
    bool runasdaemon = false;
    int threadcount = 2;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
        {
            runasdaemon = true;
        }
        else if (strcmp(argv[i], "-test") == 0)
        {
            //return test();
        }
        else if (strstr(argv[i], "-threadconn") != 0)
        {
            threadcount = atoi(argv[i] + 11);
            if (threadcount < 2)
                threadcount = 2;
            else if (threadcount > 10)
            {
                threadcount = 10;
            }
        }
        else
            printf("conn Server: Unknow parameter: %s.\n", argv[i]);
    }

    // read config
    char configpath[1024];
    Configure *config = Configure::getConfig();
    Configure::getConfigFilePath(configpath, sizeof(configpath));

    if (config->read(configpath) != 0)
    {
        exit(-1);
    }

    if (config->serverid < 0)
    {
        printf("Connection server error serverid");
        exit(1);
    }

    // init log
    logSetPrefix("Connection Server");

    // 读取服务器列表
    if (!ServerInfo::readServerInfo())
    {
        ::writelog(InfoType, "Connection Server read server info failed");
        exit(2);
    }

    //std::shared_ptr<ServerInfo> s = ServerInfo::getServerInfo(config->serverid);
    //if (!s)
    //{
    //    ::writelog("server id error.");
    //    exit(3);
    //}


    LogicalServers logicalservers;
    logicalservers.read();
    logSetPrefix("Conn Server");

    if (runasdaemon)
    {
        loginit(LogPathExternal);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    ConnectionServer cs("", config->extconn_srv_port, threadcount);
    cs.setLogicalservers(logicalservers);
    cs.run();
    return 0;
}
