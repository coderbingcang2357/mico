#include "runneraddsceneimp.h"
#include "util/logwriter.h"

RunnerAddSceneImp::RunnerAddSceneImp(std::shared_ptr<RunningSceneManager> m)
    : m_mgr(m)
{
}

void RunnerAddSceneImp::addscene(google::protobuf::RpcController *controller, const SceneRunner::AddSceneRequest *request, SceneRunner::AddSceneResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    uint64_t sceneid = request->sceneid();
    m_mgr->addScene(sceneid);
    response->set_res(0);
    ::writelog(InfoType, "add scene %llu", sceneid);
}

void RunnerAddSceneImp::removescene(google::protobuf::RpcController *controller, const SceneRunner::RemovesceneRequest *request, SceneRunner::RemovesceneResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);
    uint64_t sceneid = request->sceneid();
    m_mgr->removeScene(sceneid);
    response->set_res(0);
    ::writelog(InfoType, "remove scene %llu", sceneid);
}
