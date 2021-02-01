#include "rabbitmqservermonitor.h"
#include "newmessagecommand.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "Message/Message.h"
#include "protocoldef/protocol.h"
#include "config/configreader.h"
#include "util/logwriter.h"

LogicalServerRabbitMq::LogicalServerRabbitMq()
{

}

#if 1
bool LogicalServerRabbitMq::init()
{
	::writelog(InfoType,"rabbitmq_addr:%s",Configure::getConfig()->rabbitmq_addr.c_str());
    mq = std::make_shared<RabbitMq>(Configure::getConfig()->rabbitmq_addr, "openchannelexchange", 
			"sendrandomnumqueue", "sendrandomnumrtkey");
    bool r= mq->init();
    mq->consume([this](const std::string &m){
		//::writelog(InfoType,"8888888888888%s",m.c_str());
        this->msg(m);
    });
	return r ;
}

#endif
bool LogicalServerRabbitMq::init_alarm_notify(){
	mq_alarm_notify = std::make_shared<RabbitMq>(Configure::getConfig()->rabbitmq_addr, "openchannelexchange", 
			"alarmnotifyqueue", "alarmnotifyrtkey");
    bool r_alarm_notify = mq_alarm_notify->init();
    mq_alarm_notify->consume([this](const std::string &m){
		//::writelog(InfoType,"999999999933333%s",m.c_str());
        this->msg_alarm_notify(m);
    });

	return r_alarm_notify ;
}

void LogicalServerRabbitMq::run()
{
    mq->run();
}

#if 1
void LogicalServerRabbitMq::run_alarm_notify()
{
    mq_alarm_notify->run();
}
#endif

void LogicalServerRabbitMq::quit()
{
    mq->quit();
}

void LogicalServerRabbitMq::quit_alarm_notify()
{
    mq_alarm_notify->quit();
}

void LogicalServerRabbitMq::msg(const std::string &m)
{
    //
	Message *msg = new Message;
    InternalMessage *fmsg = new InternalMessage(msg);
    //InternalMessage *fmsg = new InternalMessage(new Message);
    fmsg->message()->setConnectionId(0);	// 0 udp,others tcp
    fmsg->message()->setConnectionServerid(0);

    //fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    fmsg->message()->setCommandID(CMD::IN_CMD_NEW_RABBITMQ_MESSAGE);
    fmsg->message()->appendContent(m);
    NewMessageCommand *ic = new NewMessageCommand(fmsg);
    ::MainQueue::postCommand(ic);
}

#if 1
void LogicalServerRabbitMq::msg_alarm_notify(const std::string &m)
{
    //
	Message *msg = new Message;
    InternalMessage *fmsg = new InternalMessage(msg);
    fmsg->message()->setConnectionId(0);	// 0 udp,others tcp
    fmsg->message()->setConnectionServerid(0);

    //fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    fmsg->message()->setCommandID(CMD::IN_CMD_NEW_RABBITMQ_MESSAGE_ALARM_NOTIFY);
    fmsg->message()->appendContent(m);
    NewMessageCommand *ic = new NewMessageCommand(fmsg);
    ::MainQueue::postCommand(ic);
}
#endif
