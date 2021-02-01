#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include "BusinessManager.h"
#include "util/logwriter.h"
#include "config/configreader.h"
#include "msgqueue/ipcmsgqueue.h"
#include "mainqueue.h"
#include "posixMessageReceiveThread.h"
#include "queuemessage.h"
#include "Message/Message.h"
#include "protocoldef/protocol.h"
#include "notifyidgen.h"

bool running = true;
bool istest = false;
bool runasdaemon = false;
void test();
int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
            runasdaemon = true;
        else if (strcmp(argv[i], "-test") == 0)
            istest = true;
        else
            printf("Notify Server: Unknow parameter: %s.\n", argv[i]);
    }

    char configpath[1024];
    Configure *config = Configure::getConfig();
    Configure::getConfigFilePath(configpath, sizeof(configpath));

    if (config->read(configpath) != 0)
    {
        exit(1);
    }

    logSetPrefix("Notify Server");
    if (runasdaemon)
    {
        loginit(LogPathNotify);
        int err = daemon(1, // don't change dir
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

#ifndef _DEBUG_VERSION
    //SetDaemon();
#endif
    Notify::genIdInit(config->serverid);

    //初始化共享内存
    if (::createMsgQueue(Notify_Logical) == -1)
    {
        ::writelog("createmsgqueue failed.");
        exit(1);
    }

    BusinessManager businessManager;
    PosixMessageReceiveThread receiveMsgFromLogicalServer;
    if (istest)
    {
        test();
    }

    QueueMessage *msg;
    while ((msg = ::getQueueMessage()) != 0 && running) // so post a null msg to queue will quit
    {
        switch (msg->type())
        {
            case QueueMessage::NewNotifyRequest:
            {
                // process notifyrequest
                NewNotifyRequestMessage *nrm
                        = dynamic_cast<NewNotifyRequestMessage *>(msg);
                businessManager.HandleIpcRequest(nrm->getReq());
            }
            break;

            case QueueMessage::NotifyTimeout:
            {
                // resend message or remove from db
                NotifyTimeoutMessage *timeoutmsg
                                = dynamic_cast<NotifyTimeoutMessage *>(msg);
                businessManager.notifyTimeout(timeoutmsg->notifyID(),
                                            timeoutmsg->times());
            }
            break;

            case QueueMessage::RemoveNotify:
            {
                // remove message from db and notify manager
                // 没有这个消息, 通知删除在 Business.notifyTimeout中和
                // NotifReqHandler中处理了
            }

            default:
                ::writelog("unknown msg type");
                assert(false);
        }

        delete msg;
        msg = 0;
    }

    return 0;
}

void sigusr1(int)
{
    running = false;
    ::postMessage(0);
}

void test()
{
    // siguser1 to exit
    sighandler_t re = signal(SIGALRM, sigusr1);
    alarm(40);
    if (re == SIG_ERR)
    {
        ::printf("signal Failed: %s", strerror(errno));
        exit(1);
    }

    Configure *config = Configure::getConfig();
    config->databasesrv_sql_host="10.1.240.39";
    config->databasesrv_sql_port=3306;
    config->databasesrv_sql_user="ctdb";
    config->databasesrv_sql_db="test";
    config->databasesrv_sql_pwd="SSDD5512";

    for (int i = 0; i < 10; ++i)
    {
        Message *msg = new Message;
        msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
        Notification notif;
        notif.Init(10, 0, CMD::NTP_USER_LOGIN_AT_ANOTHER_CLIENT, 0, 0, 0);
        msg->appendContent(&notif);
        NewNotifyRequestMessage *nrm =
                                new NewNotifyRequestMessage(msg);
        ::postMessage(nrm);
    }
    Message *msg = new Message;
    msg->setCommandID(CMD::CMD_GET_NOTIFY_WHEN_BACK_ONLINE);
    msg->setObjectID(10);
    Notification notif;
    notif.Init(10, 0, CMD::NTP_USER_LOGIN_AT_ANOTHER_CLIENT, 0, 0, 0);
    msg->appendContent(&notif);
    NewNotifyRequestMessage *nrm =
                            new NewNotifyRequestMessage(msg);
    ::postMessage(nrm);
}
