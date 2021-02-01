#include "runnerregisterclient.h"
#include "util/logwriter.h"

RunnerRegisterClient::RunnerRegisterClient(std::string serveripport)
    : m_serveripport(serveripport)
{
    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    //options.connection_type = FLAGS_connection_type;
    //options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    //options.max_retry = FLAGS_max_retry;
    if (channel.Init(m_serveripport.c_str(), "", &options) != 0) {
        ::writelog(InfoType, "Fail to initialize channel");
        return;
    }

    stub = new SceneRunner::RunnerRegisterService_Stub(&channel);
}

RunnerRegisterClient::~RunnerRegisterClient()
{
    delete stub;
}

void RunnerRegisterClient::registerrunner(int nodeid, std::string runnerip, uint16_t runnerport)
{
    brpc::Controller ctl;
    SceneRunner::RunnerRegisteRequest req;
    SceneRunner::RunnerRegisteResponse resp;
    req.set_nodeid(nodeid);
    req.set_ip(runnerip);
    req.set_port(runnerport);
    stub->registerrunner(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "register runner node failed");
    }
    else
    {
        //return resp.
        ::writelog(InfoType, "register runner node ok");
    }
}

void RunnerRegisterClient::unregisterrunner(int nodeid)
{
    brpc::Controller ctl;
    SceneRunner::RunnerUnregisterRequest req;
    SceneRunner::RunnerUnregisterResponse resp;
    req.set_nodeid(nodeid);
    stub->unregisterrunner(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "unregister runner node failed");
    }
    else
    {
        //return resp.
        ::writelog(InfoType, "unregister runner node okkkkkkkkkk");
    }
}

int RunnerRegisterClient::heartbeat(int nodeid)
{
    brpc::Controller ctl;
    SceneRunner::RunnerHeartbeatRequest req;
    SceneRunner::RunnerHeartbeatResponse resp;
    req.set_nodeid(nodeid);
    stub->heartbeat(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "runner node heart failed");
        return 0;
    }
    else
    {
        //return resp.
        //::writelog(InfoType, "runner node heartbeat okkkkkkkkkk");
        return resp.res();
    }

}
