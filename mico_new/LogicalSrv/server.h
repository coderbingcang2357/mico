#ifndef SERVER__H__
#define SERVER__H__
#include <list>
#include <functional>
#include "config/configreader.h"
#include "onlineinfo/shmcache.h"
#include "onlineinfo/onlineinfo.h"
#include "messagereceiver.h"
#include "tcpserver/tcpserver.h"
#include "udpserver.h"
#include "messagesender.h"
#include "Business/Business.h"
#include "msgqueue/procqueue.h"
#include "pushmessage/pushmessagesubscribethread.h"
#include "rabbitmqservermonitor.h"

class ICache;
class IMysqlConnPool;
class ISceneDal;
class IDelScenefromFileServer;
class IDelSceneUploadLog;
//class IMessageRepeatChecker;
class UserOperator;
class SignalThread;
class InternalMessage;
class PushService;
class SceneRunnerManager;
class RunningSceneDbaccess;

class MicoServer
{
public:
    MicoServer();
    ~MicoServer();

    int init();
    int exec();
    void quit();

    void setThreadCount(int thc);
    Configure *getconfig();
    void asnycMail(const std::function<void()> &op);
    void asnycWork(const std::function<void()> &op);
    void sendMessage(const std::list<InternalMessage *> &msgs);
    ICache *onlineCache();
    Tcpserver *tcpServer();
    UserOperator *userOperator();
    MessageSender *messageSender();
    IMysqlConnPool *mysqlconnpool(){return m_mysqlconnpool;}

private:
    void createMailQueueAndThread();
    void createAsnycQueueAndThread();
    void initMessageProcessor(InternalMessageProcessor *msgprocessor);

    SignalThread *m_sigt = nullptr;// signal wait thread
    Configure *m_config = nullptr;
    int m_threadcount = 4;
    OnLineInfo m_onlinecache; // 保存登录的 用户信息, 设备信息
    //Shmcache m_onlinecache; // 保存登录的 用户信息, 设备信息
    MessageReceiver r;
    Tcpserver *m_tcpserver = nullptr;
    Udpserver *m_udpserver = nullptr;
    MessageSender *m_msgsender = nullptr;
    InternalMessageProcessor *m_imsgprocessor = nullptr;
    // mail sender, 发送邮件的线程, 因为发邮件比较慢, 所以专门建了一个
    // 队列, mailqueue, 以有一个线程来发送邮件
    ProcQueue *m_mailqueue = nullptr;
    std::list<Thread *> m_mailsenderthread;
    // 生成通知的线程
    ProcQueue *m_asyncqueue = nullptr;
    std::list<Thread *> m_asyncthread;

    UserOperator   *m_useroperator = nullptr;
    IMysqlConnPool *m_mysqlconnpool = nullptr;

    ISceneDal *m_scenedal = nullptr;
    IDelScenefromFileServer *m_delscenefromfileserver = nullptr;
    IDelSceneUploadLog *m_deluploadlog = nullptr;

    // udp connection
    //IMessageRepeatChecker *m_udpconnection = nullptr;

    // push
    PushService *m_pushservice;
    PushMessageSubscribeThread *m_pushmsgsubscribe;
    // server run scene
    std::shared_ptr<SceneRunnerManager> m_scenerun;
    std::shared_ptr<RunningSceneDbaccess> m_rundb;
    LogicalServerRabbitMq *m_rabbitmq;
    //LogicalServerRabbitMq *m_rabbitmq_alarm_notify;
};
#endif
