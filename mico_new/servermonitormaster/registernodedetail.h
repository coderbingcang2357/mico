#pragma once
#include <stdint.h>
#include <event2/event.h>
#include "runnerregisterclient.h"
#include "runnernodeinfo.h"

class RegisterNodeDetail
{
public:
    RegisterNodeDetail(std::string serveripport, RunnerNode rn);
    ~RegisterNodeDetail();
    bool init();
    void run();
    void quit();

private:
    static void heartbeatCallback(evutil_socket_t fd, short what, void *arg);
    static void timerCallback(evutil_socket_t fd, short what, void *arg);

    void heartbeat();
    void timer();

    RunnerNode nodeinfo;

    event_base *evbase;
    struct event *emptertimer = nullptr;
    struct event *queuetimer = nullptr;
    struct event *heartbeatimer = nullptr;
    RunnerRegisterClient *client = nullptr;
};
