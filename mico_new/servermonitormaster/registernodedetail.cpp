#include "registernodedetail.h"
#include "util/logwriter.h"

RegisterNodeDetail::RegisterNodeDetail(std::string serveripport, RunnerNode rn)
{
    nodeinfo = rn;
    client = new RunnerRegisterClient(serveripport);
}

RegisterNodeDetail::~RegisterNodeDetail()
{
    delete client;
    event_free(emptertimer);
    event_free(queuetimer);
    event_free(heartbeatimer);
    event_base_free(evbase);
    ::writelog("~reg");
}

static void emptytimerCallback(evutil_socket_t fd, short what, void *arg)
{
}

void RegisterNodeDetail::heartbeatCallback(evutil_socket_t fd, short what, void *arg)
{
    RegisterNodeDetail *rm = (RegisterNodeDetail *)arg;
    rm->heartbeat();
}

void RegisterNodeDetail::timerCallback(int fd, short what, void *arg)
{
    RegisterNodeDetail *rm = (RegisterNodeDetail *)arg;
    rm->timer();
}

bool RegisterNodeDetail::init()
{
    // access to the event loop
    evbase = event_base_new();

    // queuetimer
    struct timeval ms200 = { 0, 200 * 1000 }; // 10ms
    emptertimer = event_new(evbase, -1, EV_PERSIST, emptytimerCallback, this);
    event_add(emptertimer, &ms200);


    struct timeval one_sec = { 0, 10 * 1000 }; // 10ms
    queuetimer = event_new(evbase, -1, 0, timerCallback, this);
    event_add(queuetimer, &one_sec);

    // heartbeattimer
    struct timeval sec30 = { 30, 10 * 1000 }; // 10ms
    heartbeatimer = event_new(evbase, -1, EV_PERSIST, heartbeatCallback, this);
    event_add(heartbeatimer, &sec30);
    return true;
}


void RegisterNodeDetail::run()
{
    // run the loop
    event_base_dispatch(evbase);
    ::writelog("quit~~~~~~~~~~~~~~~~~~~~");
}

void RegisterNodeDetail::quit()
{
    event_del(emptertimer);
    event_del(heartbeatimer);
    event_base_loopbreak(evbase);
    ::writelog("quit");
}

void RegisterNodeDetail::heartbeat()
{
    // send heart beat
    if (client->heartbeat(nodeinfo.nodeid) != 0)
    {
        ::writelog(InfoType, "re register ....");
        client->registerrunner(nodeinfo.nodeid, nodeinfo.ip,
                           nodeinfo.port);
    }
}

void RegisterNodeDetail::timer()
{
    //
    ::writelog("register");
    client->registerrunner(nodeinfo.nodeid, nodeinfo.ip,
                           nodeinfo.port);
}
