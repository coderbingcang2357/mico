#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "loadFromdb.h"
#include "server.h"
#include "config/configreader.h"
#include "util/logwriter.h"

bool runasdaemon = false;
int main(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-daemon") == 0)
            runasdaemon = true;
        else
            printf("unknown parameter: %s\n", argv[i]);
    }
    // load config
    char configpath[1024];
    Configure *config = Configure::getConfig();
    if (Configure::getConfigFilePath(configpath, sizeof(configpath)) != 0)
    {
        printf("find config file failed.\n");
        exit(1);
    }

    if (config->read(configpath) != 0)
    {
        exit(-1);
    }

    if (config->serverid < 0)
    {
        printf("error serverid\n");
        return 0;
    }

    logSetPrefix("LogicalServer");
    if (runasdaemon)
    {
        loginit(LogPathLogical);
        int err = daemon(1, // don't change dir
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    // 
    loadUserFromdb();
    Server server(8989, 4);
    server.run();
    return 0;
}

