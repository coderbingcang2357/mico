#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <utility>

#include "Business/BusinessManager.h"
#include "util/logwriter.h"
#include "thread/threadwrap.h"
#include "msgqueue/msgqueue.h"
#include "msgqueue/ipcmsgqueue.h"
#include "msgqueue/posixmsgqueue.h"
#include "timer/TimeElapsed.h"



bool runasdaemon = false;

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
            runasdaemon = true;
        else
            printf("ExtConnSrv: Unknow parameter: %s.\n", argv[i]);
    }

    logSetPrefix("External Server");
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


#ifndef _DEBUG_VERSION
    //daemon(1, 1);
#endif

    BusinessManager busManager;
    int sock = busManager.Init();
    // On  Linux, a message queue descriptor is actually a file descriptor, and
    // can be monitored using select(2), poll(2), or epoll(7).  This is not
    // portable
    int msgqueuefd = ::msgqueueLogicalToExtserver()->fd(); // logical 发过数据过来的队列
    int maxFdp = sock > msgqueuefd ? sock + 1 : msgqueuefd + 1;

    fd_set readfds;

    // 用来统计收包数量
    timeval starttime;
    timeval endtime;
    timeval el;
    gettimeofday(&starttime, 0);

    int recvcnt = 0;
    int msgqueucnt = 0;
    uint64_t packetcounter = 0;

    srand(time(0));

    while(1)
    {
        //1. 处理客户输入的消息
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(msgqueuefd, &readfds);

        timeval timeout = {0, 100 * 1000};// 100 ms

        int ret = select(maxFdp, &readfds, NULL, NULL, &timeout);
        if(ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                assert(false);
                exit(-1);
            }
        }
        else if (ret == 0)
        {
            // timeout
        }
        else
        {
            if(FD_ISSET(sock, &readfds))
            {
                // return number of packets processed
                recvcnt += busManager.HandleMsgInput();
            }
            if (FD_ISSET(msgqueuefd, &readfds))
            {
                msgqueucnt += busManager.HandleMsgOutput();
            }
        }
        gettimeofday(&endtime, 0);
        timersub(&endtime, &starttime, &el);
        if (el.tv_sec >= 5)
        {
            packetcounter += recvcnt;

            gettimeofday(&starttime, 0);
            //::writelog(InfoType,
            //    "all: %lu, packets: %d, msgqueue packets: %d, s: %lu, us: %lu\n",
            //    packetcounter,
            //    recvcnt,
            //    msgqueucnt,
            //    el.tv_sec,
            //    el.tv_usec);
            recvcnt = 0;
            msgqueucnt = 0;
        }
    }

    return 0;
}

