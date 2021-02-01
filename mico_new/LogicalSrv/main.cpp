#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

//#include "Config/Debug.h"
//#include "Init/SystemInit.h"
//#include "ShareMem/ShareMem.h"
#include "Business/Business.h"
//#include "Cache/CacheManager.h"
#include "onlineinfo/onlineinfo.h"
#include "onlineinfo/shmcache.h"
#include "util/logwriter.h"
#include "config/configreader.h"
#include "msgqueue/ipcmsgqueue.h"
#include "tcpserver/tcpserver.h"
#include "messageforward.h"
#include "messagereceiver.h"
#include "serverinfo/serverinfo.h"
#include "protocoldef/protocol.h"

#include "usermessageprocess.h"
#include "loginprocessor.h"
#include "deviceregister.h"
#include "devicelogin.h"
#include "userpasswordrecover.h"
#include "searchuser.h"
#include "scenemessageprocess.h"
#include "claimdevice.h"
#include "usermodifymail.h"
#include "feedback.h"
#include "messagefromnotifysrv.h"
#include "deletenotifymessage.h"
#include "transferdevicecluster.h"
#include "thread/threadwrap.h"
#include "messagesender.h"
#include "getserverlist.h"
#include "getuserinfo.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "deleteLocalCache.h"
#include "sceneconnectordevices.h"
#include "sceneConnectDevicesOperator.h"
#include "udpserver.h"
#include "newmessagecommand.h"
#include "msgqueue/procqueue.h"
#include "server.h"
#include "test/testreadallonlinefromredis.h"

int test();

int main(int argc, char* argv[])
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
            return test();
        }
        else if (strstr(argv[i], "-thread") != 0)
        {
            threadcount = atoi(argv[i] + 7);
            if (threadcount < 1)
                threadcount = 1;
            else if (threadcount > 2000)
            {
                threadcount = 100;
            }
        }
        else
            printf("Logical Server: Unknow parameter: %s.\n", argv[i]);
    }

    if (runasdaemon)
    {
        loginit(LogPathLogical);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    MicoServer ms;
    ms.setThreadCount(threadcount);
    if (ms.init() != 0)
    {
        return 1;
    }
    ms.exec();
    return 0;

    //}

    //while(MainQueue::isrun())
    //{
    //    bool idle = true;

    //    if (imsgprocessor.handleMessage() == 0)
    //        idle = false;

    //    gettimeofday(&endtime, 0);
    //    timersub(&endtime, &starttime, &el);
    //    // 统计一秒钟处理了多少数据包
    //    if (el.tv_sec >= 5)
    //    {
    //        //gettimeofday(&starttime, 0);
    //        //::writelog(InfoType,
    //        //    "packets from extserver: %d, from InternalSrv: %d, from NotifySrv: %d, from RelaySrv: %d",
    //        //    cnt, cntfromIntSrv, cntfromNotifySrv, cntfromRelaySrv);
    //        //cnt = 0;
    //        //cntfromIntSrv = 0;
    //        //cntfromNotifySrv = 0;
    //        //cntfromRelaySrv = 0;
    //    }

    //    if(idle)
    //        usleep(10000);
    //}
    //::writelog("exit!.");

    //return 0;
}

int test()
{
    Configure *config = Configure::getConfig();
    config->redis_pwd = "3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb";
    config->redis_port = 6379;
    config->redis_address = "10.1.240.39";
    ::testReadAllOnlineFromRedis();

    SceneOperatorAnalysis::test();
    return 0;
}

