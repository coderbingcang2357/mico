#include "rabbitmqservice.h"
#include <unistd.h>
#include <util/logwriter.h>
#include <config/configreader.h>

RabbitMqService *RabbitMqService::get()
{
    static RabbitMqService rmq(Configure::getConfig()->rabbitmq_addr, "openchannelexchange", "sendrandomnumqueue", "sendrandomnumrtkey");
    return &rmq;
}

RabbitMqService *RabbitMqService::get_alarm()
{
    static RabbitMqService rmq_alarm(Configure::getConfig()->rabbitmq_addr, "openchannelexchange", "alarmqueue", "alarmrtkey");
    return &rmq_alarm;
}

#if 1
RabbitMqService *RabbitMqService::get_alarm_notify()
{
    static RabbitMqService rmq_alarm_notify(Configure::getConfig()->rabbitmq_addr, "openchannelexchange", "alarmnotifyqueue", 
			"alarmnotifyrtkey");
    return &rmq_alarm_notify;
}
#endif

RabbitMqService::RabbitMqService(std::string url, std::string exchange, std::string queue, std::string rk)
{
    this->url = url;
    this->exchange = exchange;
    this->queue = queue;
    this->routkey = rk;
}

bool RabbitMqService::init()
{
    m_mq = new RabbitMq(url, exchange, queue, routkey);
    return m_mq->init();
}

void RabbitMqService::publish(const std::string &msg)
{
    m_mq->publish(msg);
}

void RabbitMqService::run()
{
    m_mq->run();
}

void RabbitMqService::quit()
{
    m_mq->quit();
}

