#pragma once
#include "rabbitmq/rbq.h"

class RabbitMqService
{
public:
    static RabbitMqService *get();
    static RabbitMqService *get_alarm();
    static RabbitMqService *get_alarm_notify();
    RabbitMqService(std::string url, std::string exchange,
                    std::string queue, std::string rk);
    bool init();
    void publish(const std::string &msg);
	void process();
    void run();
    void quit();

private:
    RabbitMq *m_mq = nullptr;
    std::string url;
    std::string exchange;
    std::string queue;
    std::string routkey;
};
