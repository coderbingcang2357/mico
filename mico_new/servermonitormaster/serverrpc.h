// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// A server to receive EchoRequest and send back EchoResponse.

#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include "scenerunnermanager.pb.h"
#include "scenerepo.h"


// Your implementation of example::EchoService
// Notice that implementing brpc::Describable grants the ability to put
// additional information in /status.
namespace SceneRunner {
class SceneManagerRpc : public IsSceneRunningService {
public:
    SceneManagerRpc(SceneRepo *repo);;
    virtual ~SceneManagerRpc() {};
    virtual void isSceneRun(::google::protobuf::RpcController *controller, const ::SceneRunner::IsSceneRunningRequest *request, ::SceneRunner::IsSceneRunningResponse *response, ::google::protobuf::Closure *done) override;
    void startRunning(::google::protobuf::RpcController *controller, const ::SceneRunner::StartRunningRequest *request, ::SceneRunner::StartRunningResponse *response, ::google::protobuf::Closure *done) override;
    void stopRunning(::google::protobuf::RpcController *controller, const ::SceneRunner::StopRunningRequest *request, ::SceneRunner::StopRunningReponse *response, ::google::protobuf::Closure *done) override;

private:
    SceneRepo *m_scenerepo;
};
}  // namespace example

