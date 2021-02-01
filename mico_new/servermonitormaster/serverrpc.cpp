#include "serverrpc.h"

SceneRunner::SceneManagerRpc::SceneManagerRpc(SceneRepo *repo)
    : m_scenerepo(repo)
{
}

void SceneRunner::SceneManagerRpc::isSceneRun(google::protobuf::RpcController *controller, const SceneRunner::IsSceneRunningRequest *request, SceneRunner::IsSceneRunningResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl =
            static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "Received request[log_id=" << cntl->log_id()
              << "] from " << cntl->remote_side()
              << " to " << cntl->local_side()
              << "sid : " << request->sceneid()
              << " (attached=" << cntl->request_attachment() << ")";

    int s = m_scenerepo->isRunning(request->sceneid());
    // Fill response.
    response->set_status(s);

}

void SceneRunner::SceneManagerRpc::startRunning(google::protobuf::RpcController *controller, const SceneRunner::StartRunningRequest *request, SceneRunner::StartRunningResponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl =
            static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "start run scene request[log_id=" << cntl->log_id()
              << "] from " << cntl->remote_side()
              << " to " << cntl->local_side()
              << "sid : " << request->sceneid()
              << " (attached=" << cntl->request_attachment() << ")";

    int res = m_scenerepo->runScene(request->sceneid());
    // Fill response.
    response->set_res(res);
}

void SceneRunner::SceneManagerRpc::stopRunning(google::protobuf::RpcController *controller, const SceneRunner::StopRunningRequest *request, SceneRunner::StopRunningReponse *response, google::protobuf::Closure *done)
{
    brpc::ClosureGuard done_guard(done);

    brpc::Controller* cntl =
            static_cast<brpc::Controller*>(controller);

    LOG(INFO) << "start scene request[log_id=" << cntl->log_id()
              << "] from " << cntl->remote_side()
              << " to " << cntl->local_side()
              << "sid : " << request->sceneid()
              << " (attached=" << cntl->request_attachment() << ")";
    int res = m_scenerepo->stopScene(request->sceneid());
    // Fill response.
    response->set_res(res);
}
