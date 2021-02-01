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

// A client sending requests to server every 1 second.

#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include "scenerunnermanager.pb.h"

//DEFINE_string(attachment, "", "Carry this along with requests");
//DEFINE_string(protocol, "baidu_std", "Protocol type. Defined in src/brpc/options.proto");
//DEFINE_string(connection_type, "", "Connection type. Available values: single, pooled, short");
//DEFINE_string(server, "0.0.0.0:8000", "IP Address of server");
//DEFINE_string(load_balancer, "", "The algorithm for load balancing");
//DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
//DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)"); 
//DEFINE_int32(interval_ms, 1000, "Milliseconds between consecutive requests");

class SceneRunnerManager
{
public:
    SceneRunnerManager(const std::string &ipport);
    ~SceneRunnerManager();
    int isRunning(uint64_t sceneid);
    int startRunning(uint64_t sceneid);
    int stopRunning(uint64_t sceneid);

private:
    brpc::Channel channel;
    //SceneRunner::IsScene stub;//(&channel);
    SceneRunner::IsSceneRunningService_Stub *stub = nullptr;
};

