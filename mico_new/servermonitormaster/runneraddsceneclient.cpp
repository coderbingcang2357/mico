#include "runneraddsceneclient.h"
#include "util/logwriter.h"

RunerAddSccneClient::RunerAddSccneClient(std::string ip, uint16_t port)
{
    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    //options.connection_type = FLAGS_connection_type;
    //options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    //options.max_retry = FLAGS_max_retry;
    std::string ipport;
    ipport.append(ip);
    ipport.append(":");
    ipport.append(std::to_string(port));
    if (channel.Init(ipport.c_str(), "", &options) != 0) {
        ::writelog(InfoType, "Fail to initialize channel");
        return;
    }
    stub = std::make_shared<SceneRunner::RunnerAddSceneService_Stub>(&channel);
}

int RunerAddSccneClient::addScene(uint64_t sceneid)
{
    brpc::Controller ctl;
    SceneRunner::AddSceneRequest req;
    SceneRunner::AddSceneResponse resp;
    req.set_sceneid(sceneid);
    stub->addscene(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "add scene failed %llu", sceneid);
        return -1;
    }
    else
    {
        //return resp.
        ::writelog(InfoType, "add scene success %llu", sceneid);
        return 0;
    }
}

int RunerAddSccneClient::removeScene(uint64_t sceneid)
{
    brpc::Controller ctl;
    SceneRunner::RemovesceneRequest req;
    SceneRunner::RemovesceneResponse resp;
    req.set_sceneid(sceneid);
    stub->removescene(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "remove scene failed %llu", sceneid);
        return -1;
    }
    else
    {
        //return resp.
        ::writelog(InfoType, "remove scene success %llu", sceneid);
        return 0;
    }
}
