#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <functional>
#include "signalthread.h"
#include "server.h"
#include "util/logwriter.h"
#include "serverinfo/serverinfo.h"
#include "msgqueue/ipcmsgqueue.h"
#include "messageforward.h"
#include "usermessageprocess.h"
#include "modifyuserhead.h"
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
#include "signalthread.h"
#include "deleteLocalCache.h"
#include "sceneconnectordevices.h"
#include "sceneConnectDevicesOperator.h"
#include "udpserver.h"
#include "newmessagecommand.h"
#include "msgqueue/procqueue.h"
#include "mysqlcli/mysqlconnpool.h"
#include "dbaccess/scene/scenedal.h"
#include "dbaccess/scene/delsceneuploadlog.h"
#include "dbaccess/scene/delscenefromfileserver.h"
#include "udpsession/messagerepeatchecker.h"
#include "realmessagerepeatechecker.h"
#include "useroperator.h"
#include "pushmessage/pushservice.h"
#include "pushmessage/newpushmessagenotify.h"
#include "removemydevice.h"
#include "deletedevicefromcluster.h"
#include "runsceneatserverprocess.h"
#include "rabbitmqmessgeprocess.h"

//class MsgProc : public Proc
//{
//public:
//    MsgProc(InternalMessageProcessor *ip);
//    void run();
//private:
//    InternalMessageProcessor *m_ip;
//};

MicoServer::MicoServer()
{
    // 忽略SIGPIPE信号
    signal(SIGPIPE, SIG_IGN);
    // 信号处理类, 这里只处理了SIGUSR1信号, 当收到这个信号后主线程就会退出
    m_sigt = new SignalThread;
}

MicoServer::~MicoServer()
{
    this->m_mailqueue->quit();
    this->m_asyncqueue->quit();
    for (auto v : m_mailsenderthread)
    {
        delete v;
    }
    this->m_asyncqueue->quit();
    for (auto v : m_asyncthread)
    {
        delete v;
    }
    delete m_sigt;
    delete m_pushmsgsubscribe;
}

int MicoServer::init()
{
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
        printf("error serverid");
        exit(1);
    }

    // init log
    logSetPrefix("LogicalServer");

    // 读取服务器列表
    if (!ServerInfo::readServerInfo())
    {
        ::writelog(InfoType, "read server info failed");
        exit(2);
    }

    if (::createMsgQueue(Notify_Logical | Relay_Logical) != 0)
    {
         // error
         ::writelog(InfoType, "Logicalserver create msgqueue failed: %s",
                    strerror(errno));
         exit(1);
    }

    if (!m_onlinecache.init())
    {
        ::writelog("onlinecache init failed.\n");
        exit(1);
    }

    // 这个类依赖消息队列, 所以一定要在createMsgQueue函数调用之后声明
    r.create();

    std::shared_ptr<ServerInfo> s = ServerInfo::getLogicalServerInfo(config->serverid);
    if (!s)
    {
        ::writelog("server id error.");
        exit(3);
    }

    // 消息转发用的tcp服务器
    m_tcpserver = new Tcpserver("",
        s->getPort(),
        new Messageforward);
    m_tcpserver->create();

    //m_udpserver = new Udpserver;
    // 
    m_msgsender = new MessageSender(m_tcpserver, &m_onlinecache);
    //m_msgsender->setUdpserver(m_udpserver);

    createMailQueueAndThread();
    createAsnycQueueAndThread();

    m_imsgprocessor = new InternalMessageProcessor(this);

    assert(m_imsgprocessor->cache() != 0);

    // mysql pool
    m_mysqlconnpool = new MysqlConnPool;
    m_useroperator = new UserOperator(m_mysqlconnpool);
    // push service
    m_pushservice = new PushService(m_mysqlconnpool);

    // scene dal
    m_scenedal = new SceneDal(m_mysqlconnpool);
    m_delscenefromfileserver = new DelSceneFromFileServer(m_scenedal);
    m_deluploadlog = new DelSceneUploadLog(m_mysqlconnpool,
                                        m_delscenefromfileserver);

    m_rundb = std::make_shared<RunningSceneDbaccess>(m_mysqlconnpool);

    initMessageProcessor(m_imsgprocessor);

    // pushmessage thread
    m_pushmsgsubscribe = new PushMessageSubscribeThread();

    // rabbitmq
    m_rabbitmq = new LogicalServerRabbitMq();
    m_rabbitmq->init();
	// 报警通知发到mico
	m_rabbitmq->init_alarm_notify();

    return 0;
}

void MicoServer::setThreadCount(int thc)
{
    m_threadcount  = thc < 0 ? m_threadcount : thc;
}

Configure *MicoServer::getconfig() 
{
    return m_config;
}

void MicoServer::asnycMail(const std::function<void()> &op)
{
    m_mailqueue->post(op);
}

void MicoServer::asnycWork(const std::function<void()> &op)
{
    m_asyncqueue->post(op);
}

void MicoServer::sendMessage(const std::list<InternalMessage *> &msgs)
{
    for (auto v : msgs)
    {
        m_msgsender->sendMessage(v);//, 0, 0, 0, 0);
        delete v;
    }
}

ICache *MicoServer::onlineCache()
{
    return &m_onlinecache;
}

Tcpserver *MicoServer::tcpServer()
{
    return m_tcpserver;
}

UserOperator *MicoServer::userOperator()
{
    return m_useroperator;
}

MessageSender *MicoServer::messageSender()
{
    return m_msgsender;
}

//void MicoServer::createUdpConnection(const sockaddr_in &addr, uint64_t clientid)
//{
//    m_udpconnection->createSession((sockaddr *)&addr, clientid);
//}
//
//void MicoServer::removeUdpConnection(const sockaddr_in &addr, uint64_t clientid)
//{
//    m_udpconnection->removeSession((sockaddr *)&addr, clientid);
//}

int MicoServer::exec()
{
    //timeval starttime;
    //timeval endtime;
    //timeval el;
    //gettimeofday(&starttime, 0);

    //int cnt = 0;
    //int cntfromIntSrv = 0;
    //int cntfromNotifySrv = 0;
    //int cntfromRelaySrv = 0;

    // 消息处理线程
    std::list<Thread *> thread;
    for (int i = 0; i < m_threadcount; i++)
    {
        thread.push_back(new Thread([this](){
            while (MainQueue::isrun())
            {
                this->m_imsgprocessor->handleMessage();
            }
        }));
    }

#if 1
    std::thread mqth([this](){
        this->m_rabbitmq->run();
    });

#endif
    std::thread mqth_alarm_notify([this](){
        this->m_rabbitmq->run_alarm_notify();
    });

    //test
    //IsRunSceneAtServiceProcess::test();
    //StartRunSceneAtServiceProcess::test();
    //StopRunSceneAtServiceProcess::test();
    while (MainQueue::isrun())
    {
        sleep(1);
        //Message *msg = m_udpserver->recv();
        //if (msg)
        //{
        //    ::writelog(InfoType,"recv cmd:%u, objid: %lu",
        //        msg->commandID(), msg->objectID());
        //    InternalMessage *imsg
        //        = new InternalMessage(msg,
        //            InternalMessage::FromExtserver,
        //            InternalMessage::Unused);
        //    //msglist.push_back(new NewMessageCommand(imsg));
        //    MainQueue::postCommand(new NewMessageCommand(imsg));
        //}
    }

    for (auto th : thread)
    {
        delete th;
    }
    m_rabbitmq->quit();
    mqth.join();

    m_rabbitmq->quit_alarm_notify();
    mqth_alarm_notify.join();

	delete m_rabbitmq;
    return 0;
}

void MicoServer::quit()
{
    // 
    MainQueue::exit();
}

void MicoServer::createMailQueueAndThread()
{
    ProcQueue *mailqueue = new ProcQueue;
    // 目前只创建一个线程
    for (int i=0; i < 1; ++i)
    {
        Thread *t = new Thread([mailqueue]() mutable {
            for (;MainQueue::isrun();)
            {
                Proc *p = mailqueue->get();
                if (p)
                {
                    ::writelog("send mail.");
                    p->run();
                    delete p;
                }
            }
            mailqueue->quit();
        });
        m_mailsenderthread.push_back(t);
    }
    m_mailqueue = mailqueue;
}

void MicoServer::createAsnycQueueAndThread()
{
    ProcQueue *queue = new ProcQueue;
    for (int i = 0; i < 10; ++i)
    {
        Thread *t = new Thread([queue]() mutable {
            for (;MainQueue::isrun();)
            {
                Proc *p = queue->get();
                if (p)
                {
                    p->run();
                    delete p;
                }
            }
            queue->quit();
        });
        m_asyncthread.push_back(t);
    }
    m_asyncqueue = queue;
}

void MicoServer::initMessageProcessor(InternalMessageProcessor *msgprocessor)
{
    ICache *cache = msgprocessor->cache();
    // 用户
    // 修改昵称
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_NICKNAME,
        std::shared_ptr<IMessageProcessor>(new ModifyNickName(cache,
                                                              m_useroperator)));

    // 修改签名
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_SIGNATURE,
        std::shared_ptr<IMessageProcessor>(new ModifySignature(cache,
                                                              m_useroperator)));

    // modify head
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_HEAD,
        std::shared_ptr<IMessageProcessor>(new ModifyUserHead(cache,
                                                              m_useroperator)));

    // 取得用户加入了哪些设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_CLUSTER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetClusterListOfUser(cache,
                                                              m_useroperator)));

    // login
    std::shared_ptr<IMessageProcessor> loginproc(new LoginProcessor(cache, this, m_useroperator, m_pushservice));
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGIN1__REQ, loginproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGIN2__VERIFY, loginproc);

    // 注销, login out CMD_PER__LOGOUT
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__LOGOUT,
        std::shared_ptr<UserLogoutProcessor>(new UserLogoutProcessor(this, m_useroperator)));

    // 用户下线
    msgprocessor->appendMessageProcessor(CMD::INCMD_USER_OFFLINE,
        std::shared_ptr<UserOffline>(new UserOffline(cache, m_useroperator)));


    // user register
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__REGISTER1__SET_MAILBOX,
        std::shared_ptr<IMessageProcessor>(new UserRegister(cache,
                                                            m_useroperator)));

    // user reset password
    std::shared_ptr<IMessageProcessor> resetpwdproc(
                                new UserPasswordRecover(this, m_useroperator));
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD1__REQ,
        resetpwdproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD2__VERIFY,
        resetpwdproc);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__RECOVER_PWD3__RESET,
        resetpwdproc);

    // user modify password CMD_PER__MODIFY_PASSWORD
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_PASSWORD,
        std::shared_ptr<UserModifyPassword>(
                             new UserModifyPassword(cache, m_useroperator)));

    // user modify mail
    std::shared_ptr<UserModifyMail> usermodifymail(new UserModifyMail(this,
                                                          m_useroperator));
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_MAILBOX1__REQ,
        usermodifymail);
    msgprocessor->appendMessageProcessor(CMD::CMD_PER__MODIFY_MAILBOX2__VERIFY,
        usermodifymail);

    // search user
    std::shared_ptr<IMessageProcessor> searchuserproc(new SearchUser(cache,
                                                            m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_FRI__ADD_FRIEND__SEARCH_USER,
        searchuserproc);
    // 用户心跳包
    msgprocessor->appendMessageProcessor(
        CMD::CMD_GEN__HEART_BEAT,
        std::shared_ptr<IMessageProcessor>(new UserHeartBeat(this,
                                                             cache,
                                                             m_useroperator)));

    // 取用户信息CMD_PER__GET_USER_INFO
    msgprocessor->appendMessageProcessor(
        CMD::CMD_PER__GET_USER_INFO,
        std::shared_ptr<IMessageProcessor>(new GetUserInfo(cache,
                                                           m_useroperator)));

    // 取服务器列表
    msgprocessor->appendMessageProcessor(CMD::CMD_GET_SERVER_LIST,
        std::shared_ptr<GetServerList>(new GetServerList));
    // 设备 -------------------------------------------------------------------
    // 取得我的设备
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__GET_MY_DEVICE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyDeviceList(cache,
                                                           m_useroperator)));
    // 修改设备notename
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__MODIFY_DEVICE_NOTENAME,
        std::shared_ptr<IMessageProcessor>(new ModifyDeviceNoteName(cache,
                                                        m_useroperator)));
    // device register
    msgprocessor->appendMessageProcessor(CMD::DCMD__REGISTER,
        std::shared_ptr<IMessageProcessor>(new DeviceRegister(m_useroperator)));

    // device login
    std::shared_ptr<IMessageProcessor> devlogin(new DeviceLoginProcessor(this,
                                                             m_useroperator));
    msgprocessor->appendMessageProcessor(CMD::DCMD__LOGIN1,
        devlogin);
    msgprocessor->appendMessageProcessor(CMD::DCMD__LOGIN2,
        devlogin);

    // device offline
    msgprocessor->appendMessageProcessor(CMD::INCMD_DEVICE_OFFLINE,
        std::shared_ptr<IMessageProcessor>(new DeviceOffline(cache,
                                                             m_useroperator)));

    // 设备心跳包
    std::shared_ptr<IMessageProcessor> devicehb(new DeviceHeartBeat(cache,
                                                        m_useroperator));
    msgprocessor->appendMessageProcessor(CMD::DCMD__SHORT_HEART_BEAT,
        devicehb);
    msgprocessor->appendMessageProcessor(CMD::DCMD__LONG_HEART_BEAT,
        devicehb);

    // 取设备在线...状态
    msgprocessor->appendMessageProcessor(CMD::CMD_DEV__GET_MY_DEVICE_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyDeviceOnlineStatus(cache,
                                                           m_useroperator)));

    // 设备群------------------------------------------------------------------
    // 创建设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__CREATE,
        std::shared_ptr<IMessageProcessor>(new CreateDeviceCluster(cache,
                                                           m_useroperator)));

    // 搜索设备群
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__JOIN__SEARCH_CLUSTER,
        std::shared_ptr<IMessageProcessor>(new SearchDeviceCluster(cache,
                                                              m_useroperator)));

    // 解散群/删除群, CMD::CMD_DCL__DELETE__REQ
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__DELETE__REQ,
       std::shared_ptr<IMessageProcessor>(new DeleteDeviceCluster(cache,
                                                              m_useroperator)));

    // 退出群CMD::CMD_DCL__EXIT:
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__EXIT,
       std::shared_ptr<IMessageProcessor>(new UserExitDeviceCluster(cache,
                                                            m_useroperator)));
    //
    // 修改设备群 notename
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__MODIFY_NOTENAME,
        std::shared_ptr<IMessageProcessor>(
                    new DeviceClusterModifyNoteName(cache, m_useroperator)));
    // 修改设备群描述
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__MODIFY_DISCRIPTION,
        std::shared_ptr<IMessageProcessor>(
            new DeviceClusterModifyDescription(cache, m_useroperator)));
    // 取得一个设备群的信息
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_CLUSTER_INFO,
        std::shared_ptr<IMessageProcessor>(new GetDeviceClusterInfo(cache,
                                                            m_useroperator)));
    // 取得设备群中的设备
    msgprocessor->appendMessageProcessor(CMD::CMD_DCL__GET_DEVICE_MEMBER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetDeviceInCluster(cache,
                                                              m_useroperator)));

    //msgprocessor->appendMessageProcessor(
    //    CMD::CMD_DCL__GET_TRANSER_DEVICE_LIST,
    //    std::shared_ptr<IMessageProcessor>(new (cache))));

    // 取得设备群中在线的设备
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_MEMBER_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetOnlineDeviceOfCluster(cache,
                                                            m_useroperator)));

    // 取群里面某一个设备的使用者CMD::CMD_DCL__GET_OPERATOR_LIST_OF_DEV_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_OPERATOR_LIST_OF_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GetOperatorListOfDeviceInCluster(cache, m_useroperator)));

    // 取群里面某一个人可以使用哪些设备case CMD::CMD_DCL__GET_DEVICE_LIST_OF_USER_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_LIST_OF_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GetDeviceListOfUserInDeviceCluster(cache, m_useroperator)));

    // 取得设备群中的成员 case CMD::CMD_DCL__GET_USER_MEMBER_LIST:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_USER_MEMBER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetUerMembersInDeviceCluster(cache, m_useroperator)));

    // CMD_DCL__GET_USER_MEMBER_ONLINE_LIST     
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_USER_MEMBER_ONLINE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetUserMemberOnlineStatusInDeviceCluster(cache, m_useroperator)));

    // 设备认领CMD:::
    std::shared_ptr<IMessageProcessor> claimprocessor(new ClaimDeviceProcessor(cache, m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CCMD__DEVICE_CLAIM,
        claimprocessor);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_CLAIM_CHECK_PASSWORD,
        claimprocessor);

    msgprocessor->appendMessageProcessor(
        CMD::INTERNAL_CLAIM_TIMER_RESEND,
        claimprocessor);

    // 移交设备CMD::CMD_DCL__TRANSFER_DEVICES__REQ:CMD::CMD_DCL__TRANSFER_DEVICES__APPROVAL:
    std::shared_ptr<IMessageProcessor> transferdev(new TransferDeiceToAnotherClusterProcessor(cache, m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVICES__REQ,
        transferdev);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVICES__APPROVAL,
        transferdev);

    // 设备授权CMD::CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER:
    // 把一些设备给某一个人用
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__ASSIGN_DEVICES_TO_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new AssignDevicetToUser(cache, m_useroperator)));

    // 取消设备授权CMD::CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_DEVICES_OF_USER_MEMBER,
        std::shared_ptr<IMessageProcessor>(new RemoveDeviceOfUserMember(cache, m_useroperator)));

    // 用户删除我的设备
    msgprocessor->appendMessageProcessor(
                CMD::CMD_DCL__REMOVE_MY_DEVICES,
        std::shared_ptr<IMessageProcessor>(new RemoveMyDevice(cache, m_useroperator)));

    // CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER
    // 把一个设备给一些人用
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__ASSIGN_OPERATORS_TO_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new GiveDeviceToManyUser(cache, m_useroperator)));

    // CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER
    // 取消授权, 取消一个设备的一些使用者
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_OPERATORS_OF_DEV_MEMBER,
        std::shared_ptr<IMessageProcessor>(new RemoveDeviceUsers(cache, m_useroperator)));

    // 邀请加入群CMD::CMD_DCL__INVITE_TO_JOIN__REQ:CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL:
    auto inviteprocessor = std::make_shared<InviteUserToDeviceCluster>(cache, m_useroperator);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__INVITE_TO_JOIN__REQ, inviteprocessor);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__INVITE_TO_JOIN__APPROVAL, inviteprocessor);

    // 移出群成员CMD::CMD_DCL__REMOVE_USER_MEMBERS:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_USER_MEMBERS,
        std::shared_ptr<IMessageProcessor>(new RemoveUserFromDeviceCluster(cache, m_useroperator)));

    // 设为管理员CMD::CMD_DCL__MODIFY_USER_MEMBER_ROLE:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__MODIFY_USER_MEMBER_ROLE,
        std::shared_ptr<IMessageProcessor>(new ModifyUserMemberRole(cache, m_useroperator)));

    // 移交群
    std::shared_ptr<IMessageProcessor> transferDevCluster(new TransferDeviceCluster(cache, m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_REQ,
        transferDevCluster);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__TRANSFER_DEVCLUSTER_VERIRY,
        transferDevCluster);

    // 取得设备信息CMD::CMD_DCL__GET_DEVICE_INFO:
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__GET_DEVICE_INFO,
        std::shared_ptr<IMessageProcessor>(new GetDeviceInfoInCluster(cache, m_useroperator)));

    // 申请加入群CMD::CMD_DCL__JOIN__REQ:CMD::CMD_DCL__JOIN__APPROVAL:
    std::shared_ptr<IMessageProcessor> userjoincluster(new UserApplyToJoinCluster(cache, m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__JOIN__REQ,
        userjoincluster);
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__JOIN__APPROVAL,
        userjoincluster);

    // 删除设备群中的设备
    std::shared_ptr<IMessageProcessor> deldevincluster(new DeleteDeviceFromCluster(cache, m_useroperator));
    msgprocessor->appendMessageProcessor(
        CMD::CMD_DCL__REMOVE_DEVICES_FROM_DEVICE_CLUSTER,
        deldevincluster);

    // 场景相关----------------------------------------------------------------
    // get my scene list
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_MY_SCENE_LIST,
        std::shared_ptr<IMessageProcessor>(new GetMyHmiSenceListProcessor(cache, m_useroperator)));
    // modify scene info
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__MODIFY_SCENE_INFO,
        std::shared_ptr<IMessageProcessor>(new ModifySceneInfo(cache, m_useroperator)));
    // modify scene notename
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__MODIFY_NOTENAME,
        std::shared_ptr<IMessageProcessor>(new ModifyNotename(cache, m_useroperator)));
    // 取得场景信息
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_SCENE_INFO,
        std::shared_ptr<IMessageProcessor>(new GetHmiSceneInfo(cache, m_useroperator)));
    // 取得场景共享给了哪些人
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_SCENE_USER_LIST,
        std::shared_ptr<IMessageProcessor>(new GetHmiSenceShareUsers(cache, m_useroperator)));
    // 共享场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__SHARE_SCENE,
        std::shared_ptr<IMessageProcessor>(new ShareScene(cache, m_scenedal, m_useroperator)));
    // 取消共享
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__CANCEL_SHARE_SCENE,
        std::shared_ptr<IMessageProcessor>(new DonotShareScene(cache, m_useroperator)));
    // 修改共享权限
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__SET_SHARE_SCENE_AUTH,
        std::shared_ptr<IMessageProcessor>(new SetShareSceneAuth(cache, m_useroperator)));
    // 删除场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__DELETE_SCENE,
        std::shared_ptr<IMessageProcessor>(new DeleteScene(cache, m_useroperator,m_rundb.get())));

    // 移交场景CMD_HMI__TRANSFER_SYSADMIN
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__TRANSFER_SYSADMIN,
        std::shared_ptr<IMessageProcessor>(new TransferScene(cache, m_useroperator)));

    // 穿透CMD::CMD_HMI__PENETRATE__REQ
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__PENETRATE__REQ,
        std::shared_ptr<IMessageProcessor>(new HmiPenetrate(cache, m_useroperator)));
    // CMD_HMI__OPEN_RELAY_CHANNELS
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__OPEN_RELAY_CHANNELS,
        std::shared_ptr<IMessageProcessor>(new HmiRelay(cache, m_useroperator)));
    // CMD_HMI__GET_NEW_CDSESSION_KEY
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_NEW_CDSESSION_KEY,
        std::shared_ptr<IMessageProcessor>(new GetClientDeviceSessionkey(cache, m_useroperator)));
    // 取客户端外网IP/port CMD_HMI__GET_EXTRANET_IP_PORT
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__GET_EXTRANET_IP_PORT,
        std::shared_ptr<IMessageProcessor>(new GetClientIPAndPort(cache, m_useroperator)));
    //
    // 上传场景
    std::shared_ptr<UploadScene> _uploadscene(new UploadScene(this, cache, m_deluploadlog, m_useroperator));
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__UPLOAD_SCENE,
                    _uploadscene);
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__UPLOAD_SCENE_CONFIRM,
                    _uploadscene);
    // 下载场景
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__DOWNLOAD_SCENE,
        std::shared_ptr<IMessageProcessor>(new DownloadScene(cache, m_useroperator)));

    // 服务端场景监控运行
	std::string masternode_ip = Configure::getConfig()->masternodeip + ":" + Configure::getConfig()->masternode_port;
    m_scenerun = std::make_shared<SceneRunnerManager>(masternode_ip);
    //m_rundb = std::make_shared<RunningSceneDbaccess>(m_mysqlconnpool);
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI_IS_SCENE_RUN_AT_SERVER,
        std::shared_ptr<IMessageProcessor>(new IsRunSceneAtServiceProcess(cache, m_scenerun.get(), m_rundb.get())));
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI_START_SCENE_RUN_AT_SERVER,
        std::shared_ptr<IMessageProcessor>(new StartRunSceneAtServiceProcess(cache, m_scenerun.get(), m_rundb.get(),m_useroperator)));
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI_STOP_SCENE_RUN_AT_SERVER,
        std::shared_ptr<IMessageProcessor>(new StopRunSceneAtServiceProcess(cache,  m_scenerun.get(), m_rundb.get(),m_useroperator)));


    // 服务器收到此命令后给场景的所有共享者发送通知,告知场景文件被修改需要从新下载
    msgprocessor->appendMessageProcessor(CMD::CMD_SCENE_FILE_CHANGED,
        std::shared_ptr<IMessageProcessor>(new SceneFileChanged(cache, m_useroperator)));
    //
    //
    // 从relayserver过来的打开转发通道处理结果的消息
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__OPEN_RELAY_CHANNELS_RESULT,
        std::shared_ptr<IMessageProcessor>(new HmiOpenRelayResult(cache, m_useroperator)));
    // CMD_HMI__CLOSE_RELAY_CHANNELS
    msgprocessor->appendMessageProcessor(CMD::CMD_HMI__CLOSE_RELAY_CHANNELS,
        std::shared_ptr<IMessageProcessor>(new CloseRelay(cache)));

    // CMD_GEN__FEEDBACK
    msgprocessor->appendMessageProcessor(CMD::CMD_GEN__FEEDBACK,
        std::shared_ptr<IMessageProcessor>(new SaveFeedBack(cache, m_useroperator)));

    // 通知处理
    msgprocessor->appendMessageProcessor(CMD::CMD_SEND_NOTIF,
        std::shared_ptr<IMessageProcessor>(new SendNotify()));

    // 客户端收到通知后的回复处理
    std::shared_ptr<IMessageProcessor> delnotify(new DeleteNotify(cache, m_pushservice));
    msgprocessor->appendMessageProcessor(CMD::CMD_GEN__NOTIFY,
        delnotify);
    msgprocessor->appendMessageProcessor(CMD::CMD_NOTIFY_RESP,
        delnotify);

    // 
    msgprocessor->appendMessageProcessor(CMD::CMD_DEL_LOCAL_CACHE,
        std::make_shared<DeleteLocalCache>(cache));

    // 
    SceneConnectDevicesOperator *scenedevop = new SceneConnectDevicesOperator(m_useroperator);
    std::shared_ptr<SceneConnectorDeviceMessageProcessor> sceneConnDevOp
        = std::make_shared<SceneConnectorDeviceMessageProcessor>(cache, scenedevop,m_rundb.get());
    msgprocessor->appendMessageProcessor(CMD::CMD_SET_SCENE_CONNECTOR_DEVICES,
        sceneConnDevOp);
    msgprocessor->appendMessageProcessor(CMD::CMD_GET_SCENE_CONNECTOR_DEVICES,
        sceneConnDevOp);

    // push message from micom server
    std::shared_ptr<NewPushMessageNotify> npushmsg = std::make_shared<NewPushMessageNotify>(this, m_pushservice);
    msgprocessor->appendMessageProcessor(CMD::IN_CMD_NEW_PUSH_MESSAGE,
        npushmsg);

    // 开启设备通道
    std::shared_ptr<RabbitmqMessageProcess> rabbitmq = std::make_shared<RabbitmqMessageProcess>();
    msgprocessor->appendMessageProcessor(CMD::IN_CMD_NEW_RABBITMQ_MESSAGE,
        rabbitmq);
	
	// 报警信息通知场景所有者和共享者 
    std::shared_ptr<RabbitmqMessageAlarmNotifyProcess> rabbitmq_alarm_notify = 
		std::make_shared<RabbitmqMessageAlarmNotifyProcess>(m_mysqlconnpool);
    msgprocessor->appendMessageProcessor(CMD::IN_CMD_NEW_RABBITMQ_MESSAGE_ALARM_NOTIFY,
        rabbitmq_alarm_notify);
}

//MsgProc::MsgProc(InternalMessageProcessor *ip) : m_ip(ip)
//{
//}
//
//void MsgProc::run()
//{
//    while (MainQueue::isrun())
//    {
//        m_ip->handleMessage();
//    }
//}

