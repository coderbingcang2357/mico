#pragma once
#include <stdint.h>
#include <brpc/channel.h>
#include <memory>
#include "runneraddscene.pb.h"

class RunerAddSccneClient
{
public:
    RunerAddSccneClient(std::string ip, uint16_t port);
    int addScene(uint64_t sceneid);
    int removeScene(uint64_t sceneid);

private:
    std::string m_ip;
    uint16_t m_port = 0;
    brpc::Channel channel;
    //SceneRunner::IsScene stub;//(&channel);
    std::shared_ptr<SceneRunner::RunnerAddSceneService_Stub> stub = nullptr;
};
