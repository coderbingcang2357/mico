#pragma once
#include "rabbitmq/rbq.h"
#include <memory>

class LogicalServerRabbitMq
{
public:
    LogicalServerRabbitMq();
    bool init();
    void run();
    void quit();
    bool init_alarm_notify();
    void run_alarm_notify();
    void quit_alarm_notify();

private:
    void msg(const std::string &m);
    void msg_alarm_notify(const std::string &m);

    std::shared_ptr<RabbitMq> mq;
    std::shared_ptr<RabbitMq> mq_alarm_notify;
};
