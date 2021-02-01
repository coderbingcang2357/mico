#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

#include "RelayReqHandler.h"
#include "ChannelManager.h"
#include "config/shmconfig.h"
#include "util/logwriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include <string.h>
#include "mainqueue.h"
#include "relayinternalmessage.h"
#include "relaythread.h"
#include "config/configreader.h"
#include "Message/Message.h"
#include "Message/relaymessage.h"
#include "messagethread.h"
#include "thread/threadwrap.h"
#include "sessionRandomNumber.h"
#include "relayports.h"
#include "channelDatabase.h"
#include "mysqlcli/mysqlconnpool.h"

void test(uint64_t uid, uint64_t did);
void testInit();
void test();
void testclosechannel();
void stressTest();

bool runasdaemon = false;
bool istest = false;
void loadChannelFromDatabase(const std::list<std::shared_ptr<ChannelInfo>> &portindb, RelayPorts *relayports, RelayThread *relaythread);

int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-daemon") == 0)
            runasdaemon = true;
        else if (strcmp(argv[i], "-test") == 0)
            istest = true;
        else
            printf("Relay Server : Unknow parameter: %s.\n", argv[i]);
    }

    char configpath[1024];
    Configure *config = Configure::getConfig();
    Configure::getConfigFilePath(configpath, sizeof(configpath));

    if (config->read(configpath) != 0)
    {
        exit(-1);
    }

    if (config->serverid < 0)
    {
        printf("error serverid");
        return 0;
    }


    logSetPrefix("Relay Server");
    if (runasdaemon)
    {
        loginit(LogPathRelay);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

    // 初始化随机数生成器
    ::initialRandomnumberGenerator(config->serverid);

    //初始化共享内存
    if (::createMsgQueue(Relay_Logical) == -1)
    {
        ::writelog("createmsgqueue failed.");
        exit(1);
    }

    RelayPorts relayports;
    RelayThread relaythread;
    if (!relaythread.create())
    {
        ::writelog("create relay thread failed.");
        exit(2);
    }
    // 预创建32个转发通道, 因为一个设备最多32个连接
    for (int cnt = 0;cnt < 32;)
    {
        std::pair<bool, int> ports = relaythread.createRelaySocket(cnt + 20000);
        if (ports.first) // isuccess
        {
            int sockfd = ports.second;
            assert(sockfd);
            relayports.add(sockfd, cnt + 20000);
            cnt++;
        }
        // else continue;
    }

    MessageThread mt;
    if (!mt.create())
    {
        ::writelog("create msg thread failed.");
        exit(3);
    }
    // testtttttttttttttttttttttttttttt
    // test();
    //testclosechannel();
    //test();
    //uint64_t uid = 1000;
    //uint64_t did = 1000;
    Thread *testthread=0;
    if (istest)
    {
        testInit();
        test();
        return 0;
        //testthread = new Thread([&uid, &did]() {
        //    test(uid+1, did+1);
        //    sleep(5);
        //    test(uid+2, did+2);
        //    sleep(5);
        //    test(uid+1, did+1);
        //    sleep(5);
        //    test(uid+2, did+2);
        //    //for (;;)
        //    //{
        //    //    test(uid++, did++);
        //    //    if (uid > 3500)
        //    //    {
        //    //        uid = 1000;
        //    //    }
        //    //    if (did > 3500)
        //    //    {
        //    //        did = 1000;
        //    //    }
        //    //    usleep(10 * 1000); // 10 ms
        //    //}
        //});
        //(void)testthread;
    }
    // write channel to database
    MysqlConnPool mysqlpool;
    ChannelDatabase chdb(&mysqlpool);

    loadChannelFromDatabase(chdb.readChannel(), &relayports, &relaythread);

    RelayReqHandler reqHandler(&relayports);
    RelayInterMessage *rimsg;
    while ((rimsg = MainQueue::getMsg()) != 0)
    {
        // 
        switch (rimsg->type())
        {
        case RelayInterMessage::NewMessageFromOtherServer:
        {
            MessageFromOtherServer *msg = dynamic_cast<MessageFromOtherServer *>(rimsg);
            if (msg)
            {
                Message *relaymsg = msg->getMsg();
                reqHandler.handle(relaymsg);
            }
            delete rimsg;
        }
        break;

        case RelayInterMessage::ReleasePort:
        {
            MessageReleasePort *msg = dynamic_cast<MessageReleasePort*>(rimsg);
            if (msg)
            {
                relayports.release(msg->userID(), msg->deviceID(), msg->channelid());
            }
            delete msg;
        }
        break;

        case RelayInterMessage::OpenChannel:
        {
            MessageOpenChannel *msg = dynamic_cast<MessageOpenChannel *>(rimsg);
            if (msg)
            {
                auto ci = msg->channelinfo();
                RelayThreadCommand *c = new RelayThreadCommand;
                c->commandid = RelayThreadCommand::AddChannel;
                c->channel = msg->channelinfo();

                relaythread.addCommand(c);
            }
            delete rimsg;
        }
        break;

        case RelayInterMessage::CloseChannel:
        {
            MessageCloseChannel *msg = dynamic_cast<MessageCloseChannel *>(rimsg);
            if (msg)
            {
                RelayThreadCommand *c = new RelayThreadCommand;
                c->commandid = RelayThreadCommand::CloseChannel;
                c->userid = msg->userid();
                c->devid = msg->devid();

                relaythread.addCommand(c);
            }
            delete rimsg;
        }
        break;

        case RelayInterMessage::ChannelTimeOut:
        {
            //MessageChannelTimeout *msg = dynamic_cast<MessageChannelTimeout *>(rimsg);
            //if (msg)
            //{
            //    uint64_t userid = msg->userid();
            //    uint64_t devid = msg->deviceid();
            //    ::writelog(InfoType, "!close channel: %lu, %lu", userid, devid);
            //    channelManager.closeChannel(userid, devid);
            //}
            delete rimsg;
            assert(false);

        }
        break;

        case RelayInterMessage::InsertChannelToDatabase:
        {
            MessageInsertChannelToDatabase *msg = static_cast<MessageInsertChannelToDatabase *>(rimsg);
            chdb.insertChannel(msg->userid(), msg->deviceid(),
                msg->userport(), msg->deviceport(),
                msg->userfd(), msg->devicefd(),
                msg->useraddress(), msg->deviceaddress());
        }
        delete rimsg;
        break;

        case RelayInterMessage::RemoveChannelFromDatabase:
        {
            MessageRemoveChannelFromDatabase *msg = static_cast<MessageRemoveChannelFromDatabase *>(rimsg);
            chdb.removeChannel(msg->getUserID(), msg->getDeviceID());
        }
        delete rimsg;
        break;

        default:
            assert(false);
            delete rimsg;
        break;
        }
    }
    delete testthread;
    return 0;
}

void loadChannelFromDatabase(
    const std::list<std::shared_ptr<ChannelInfo>> &channelsindb,
    RelayPorts *relayports,
    RelayThread *relaythread)
{
    ::writelog("load channel from database ....");
    int chid = 1;
    for (auto ch : channelsindb)
    {
        PortPair pp;
        pp.userfd = ch->userfd;
        pp.devicefd = ch->devfd;
        pp.userport = ch->userport;
        pp.devport = ch->deviceport;
        pp.channelid = chid;
        ch->channelid = chid;
        relayports->insert(ch->userID, ch->deviceID, pp);

        RelayThreadCommand *cmd = new RelayThreadCommand;
        cmd->commandid = RelayThreadCommand::InsertChannel;
        cmd->channel = ch;
        relaythread->addCommand(cmd);
        chid++;
    }
}

void sigalrm(int)
{
    MainQueue::quit();
}

void testInit()
{
    sighandler_t re = signal(SIGALRM, sigalrm);
    if (re == SIG_ERR)
    {
        ::printf("signal Failed: %s", strerror(errno));
        exit(1);
    }
    // quit after 40s
    alarm(40);
}

void test(uint64_t uid, uint64_t did)
{
    Configure *c = Configure::getConfig();
    Message *msg = new Message;
    RelayMessage rmsg;
    rmsg.setCommandCode(Relay::OPEN_CHANNEL);
    rmsg.setUserid(uid);
    sockaddr_in useraddr;
    memset(&useraddr, 0, sizeof(useraddr));
    useraddr.sin_family = AF_INET;
    useraddr.sin_port = htons(9090);
    inet_pton(AF_INET, c->localip.c_str(), &useraddr.sin_addr);
    sockaddrwrap addrwrap;
    addrwrap.setAddress((sockaddr *)&useraddr, sizeof(useraddr));
    rmsg.setUserSockaddr(addrwrap);

    RelayDevice rd;
    rd.devicdid = did;
    useraddr.sin_port = htons(9091);
    addrwrap.setAddress((sockaddr *)&useraddr, sizeof(useraddr));
    rd.devicesockaddr = addrwrap;

    rmsg.appendRelayDevice(rd);

    msg->appendContent(&rmsg);

    MainQueue::postMsg(new MessageFromOtherServer(msg));
    //rmsg.setRelayServeraddr();

}
void test()
{
    RelayPorts::test();
    //test(10000, 10001);
}

void testclosechannel()
{
    Configure *c = Configure::getConfig();
    Message *msg = new Message;
    RelayMessage rmsg;
    rmsg.setCommandCode(Relay::CLOSE_CHANNEL);
    rmsg.setUserid(1000);
    sockaddr_in useraddr;
    memset(&useraddr, 0, sizeof(useraddr));
    useraddr.sin_family = AF_INET;
    useraddr.sin_port = htons(9090);
    inet_pton(AF_INET, c->localip.c_str(), &useraddr.sin_addr);
    sockaddrwrap addrwrap;
    addrwrap.setAddress((sockaddr *)&useraddr, sizeof(useraddr));
    rmsg.setUserSockaddr(addrwrap);

    RelayDevice rd;
    rd.devicdid = 10001;
    useraddr.sin_port = htons(9091);
    addrwrap.setAddress((sockaddr *)&useraddr, sizeof(useraddr));
    rd.devicesockaddr = addrwrap;

    rmsg.appendRelayDevice(rd);

    msg->appendContent(&rmsg);

    MainQueue::postMsg(new MessageFromOtherServer(msg));
}

