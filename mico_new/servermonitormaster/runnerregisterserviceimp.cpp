#include "runnerregisterserviceimp.h"
#include "runnernodemanager.h"
#include "util/logwriter.h"

RunnerRegisterServiceImp::RunnerRegisterServiceImp(RunnernodeManager *rm)
    : m_rm(rm)
{
}

void RunnerRegisterServiceImp::registerrunner(google::protobuf::RpcController *controller, const SceneRunner::RunnerRegisteRequest *request, SceneRunner::RunnerRegisteResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    RunnerNode rn;
    rn.ip = request->ip();
    rn.port = request->port();
    rn.hearttime = time(nullptr);
    rn.nodeid = request->nodeid();
    ::writelog(InfoType, "newnode: %s:%d nodeid: %d", rn.ip.c_str(), rn.port, rn.nodeid);
    m_rm->registerNode(rn);
    response->set_res(0);
}

void RunnerRegisterServiceImp::heartbeat(google::protobuf::RpcController *controller, const SceneRunner::RunnerHeartbeatRequest *request, SceneRunner::RunnerHeartbeatResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    RunnerNode rn;
    rn.nodeid = request->nodeid();
    int r = m_rm->nodeHeartbeat(rn);
    response->set_res(r);
}

void RunnerRegisterServiceImp::unregisterrunner(google::protobuf::RpcController *controller, const SceneRunner::RunnerUnregisterRequest *request, SceneRunner::RunnerUnregisterResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    RunnerNode rn;
    rn.nodeid = request->nodeid();
    m_rm->unregisterNode(rn);
    response->set_res(0);
}

