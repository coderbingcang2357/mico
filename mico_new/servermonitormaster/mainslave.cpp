#include "./serverscenerunnermasterclient.h"
#include <iostream>
#include <QCoreApplication>
#include <QDialog>

#include <butil/logging.h>
#include <brpc/server.h>
#include "registernodedetail.h"
#include "runneraddsceneimp.h"
#include "dbaccess/runningscenedb.h"
#include "mysqlcli/mysqlconnpool.h"
#include <thread>
#include "rabbitmqservice.h"
#include "RelaySrv/sessionRandomNumber.h"
#include "serverudp/serverudp.h"
//#include "config/configreader.h"
#include "scenerunner.h"
#include "config/configreader.h"
#include "util/logwriter.h"

void test();
int main(int argc, char* argv[]) {
    bool runasdaemon = false;

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
	}

    if (runasdaemon)
    {
        loginit(scenerunner);
        int err = daemon(1, // don't change dic
                1); // don't redirect stdin, stdout, stderr
        if (err == -1)
        {
            ::writelog(InfoType, "Daemon Failed: %s", strerror(errno));
            exit(1);
        }
    }

	//read config
	char configpath[1024];
	Configure *config = Configure::getConfig();
	Configure::getConfigFilePath(configpath, sizeof(configpath));

	if (config->read(configpath) != 0)
	{
		exit(-1);
	}

	RunnerNode rn;
    rn.ip = Configure::getConfig()->slavenodeip;
#if 0
	std::string nodeport;
	if(!Configure::getConfig()->getSlaveNodePort(argv[1], &nodeport)) {
		::writelog(InfoType,"argv parament error!");
		return -1;
	}
	rn.port = std::stoi(nodeport);
#endif
	std::string nodeport = argv[1];
	rn.port = std::stoi(nodeport);

	uint32_t nodeid;
	::getNodeId(rn.port, &nodeid);

	std::string prefix = "Scenerunner Server" + std::to_string(nodeid);
    logSetPrefix(prefix.c_str());
    // 开启通道会用到
    ::initialRandomnumberGenerator(nodeid);
    rn.nodeid = nodeid;

    // Generally you only need one Server.
    brpc::Server server;

	std::shared_ptr<MysqlConnPool> pool(new MysqlConnPool);
	//IMysqlConnPool *pool = new MysqlConnPool;
    std::shared_ptr<RunningSceneDbaccess> dba =
            std::make_shared<RunningSceneDbaccess>(pool.get());
    std::shared_ptr<RunningSceneManager> manager =
            std::make_shared<RunningSceneManager>(nodeid, dba);
    RunnerAddSceneImp addsceneservice(manager);
    	//std::cout<<"slavenode_port1"<<Configure::getConfig()->slavenode_port1<<std::endl;
    //rn.port = std::stoi(Configure::getConfig()->slavenode_port1);
	std::string master_ip_port = Configure::getConfig()->masternodeip + ":" + Configure::getConfig()->masternode_port;
    RegisterNodeDetail nodedetail(
                master_ip_port, rn);
    nodedetail.init();
    std::thread th([&nodedetail](){
        nodedetail.run();
    });

#if 1
    int ac = argc;
    std::mutex wait;
    wait.lock();
    QCoreApplication *appptr = nullptr;
    std::thread th2([&wait,&appptr, &ac, &manager, argv](){
        QCoreApplication app(ac, argv);
        appptr = &app;
        //QDialog dlg;
        //dlg.show();
        manager->reloadFromDatabase();
        wait.unlock();
        app.exec();
        //delete app;
        appptr = nullptr;
    });
    wait.lock();
#endif
	// 打开通道
	RabbitMqService::get()->init();
    // run mq in a thread
	std::thread thmq([](){
			RabbitMqService::get()->run();
    });

	// 报警数据传给微信公众号开发服务器
	RabbitMqService::get_alarm()->init();
    // run mq in a thread
	std::thread thmq_alarm([](){
			RabbitMqService::get_alarm()->run();
    });

	RabbitMqService::get_alarm_notify()->init();
    // run mq in a thread
	std::thread thmq_alarm_notify([](){
			RabbitMqService::get_alarm_notify()->run();
    });

    //test();
    // Instance of your service.
    // SceneRunner::SceneManagerRpc serverrpc;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.

    if (server.AddService(&addsceneservice,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    if (server.Start(std::stoi(nodeport), &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();

    nodedetail.quit();
    th.join();

#if 1
    if (appptr != nullptr)
        appptr->quit();
    th2.join();
#endif

    RabbitMqService::get_alarm()->quit();
    thmq_alarm.join();

    RabbitMqService::get_alarm_notify()->quit();
    thmq_alarm_notify.join();

    RabbitMqService::get()->quit();
    thmq.join();
    return 0;
}

void test()
{
    RunningSceneData rd;
    SceneRun::runScene(rd);
    //ServerUdp u(2, 3, "127.0.0.1");
    //u.bind();
    //u.openChannel();
}
