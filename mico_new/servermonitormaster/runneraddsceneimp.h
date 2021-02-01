#pragma once
#include <brpc/server.h>
#include <memory>
#include "runneraddscene.pb.h"
#include "runningscenemanager.h"

class RunnerAddSceneImp : public SceneRunner::RunnerAddSceneService
{
public:
    RunnerAddSceneImp(std::shared_ptr<RunningSceneManager> m);
    void addscene(::google::protobuf::RpcController *controller, const ::SceneRunner::AddSceneRequest *request, ::SceneRunner::AddSceneResponse *response, ::google::protobuf::Closure *done) override;
    void removescene(::google::protobuf::RpcController *controller, const ::SceneRunner::RemovesceneRequest *request, ::SceneRunner::RemovesceneResponse *response, ::google::protobuf::Closure *done) override;

private:
    std::shared_ptr<RunningSceneManager> m_mgr;
};
