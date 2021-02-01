#include "serverscenerunnermasterclient.h"
#include "util/logwriter.h"

SceneRunnerManager::SceneRunnerManager(const std::string &ipport)
{
    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    //options.connection_type = FLAGS_connection_type;
    //options.timeout_ms = FLAGS_timeout_ms/*milliseconds*/;
    //options.max_retry = FLAGS_max_retry;
    if (channel.Init(ipport.c_str(), "", &options) != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return;
    }

    stub = new SceneRunner::IsSceneRunningService_Stub(&channel);
}

SceneRunnerManager::~SceneRunnerManager()
{
    delete stub;
}

int SceneRunnerManager::isRunning(uint64_t sceneid)
{
    brpc::Controller ctl;
    SceneRunner::IsSceneRunningRequest req;
    SceneRunner::IsSceneRunningResponse resp;
    req.set_sceneid(sceneid);
    stub->isSceneRun(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
		::writelog(InfoType,"isRunning failed!");
        return 0;
    }
    else
    {
        return resp.status();
    }
}

int SceneRunnerManager::startRunning(uint64_t sceneid)
{
    brpc::Controller ctl;
    SceneRunner::StartRunningRequest req;
    SceneRunner::StartRunningResponse resp;
    req.set_sceneid(sceneid);
    stub->startRunning(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
        ::writelog(InfoType, "start running rpc failed: %lu", sceneid);
        return -1;
    }
    else
    {
        return resp.res();
    }
}

int SceneRunnerManager::stopRunning(uint64_t sceneid)
{
    brpc::Controller ctl;
    SceneRunner::StopRunningRequest req;
    SceneRunner::StopRunningReponse resp;
    req.set_sceneid(sceneid);
    stub->stopRunning(&ctl, &req, &resp, nullptr);
    if (ctl.Failed())
    {
        // log here
		::writelog(InfoType, "stopRunning failed!");
        return 0;
    }
    else
    {
        return resp.res();
    }
}
