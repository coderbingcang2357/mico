#pragma once
#include <brpc/server.h>
#include "runnerregister.pb.h"
class RunnernodeManager;
class RunnerRegisterServiceImp : public SceneRunner::RunnerRegisterService
{
public:
    RunnerRegisterServiceImp(RunnernodeManager *rm);
    void registerrunner(::google::protobuf::RpcController* controller, const ::SceneRunner::RunnerRegisteRequest* request, ::SceneRunner::RunnerRegisteResponse* response, ::google::protobuf::Closure* done) override;
    void heartbeat(::google::protobuf::RpcController *controller, const ::SceneRunner::RunnerHeartbeatRequest *request, ::SceneRunner::RunnerHeartbeatResponse *response, ::google::protobuf::Closure *done) override;
    void unregisterrunner(::google::protobuf::RpcController *controller, const ::SceneRunner::RunnerUnregisterRequest *request, ::SceneRunner::RunnerUnregisterResponse *response, ::google::protobuf::Closure *done) override;

private:
    RunnernodeManager *m_rm;
};
