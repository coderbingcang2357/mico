#pragma once
#include <string>
#include <stdint.h>
#include <brpc/channel.h>
#include "runnerregister.pb.h"

class RunnerRegisterClient
{
public:
    RunnerRegisterClient(std::string serveripport);
    ~RunnerRegisterClient();
    void registerrunner(int nodeid,
                        std::string runnerip,
                        uint16_t runnerport);
    void unregisterrunner(int nodeid);
    int heartbeat(int nodeid);

private:
    std::string m_serveripport;
    brpc::Channel channel;
    //SceneRunner::IsScene stub;//(&channel);
    SceneRunner::RunnerRegisterService_Stub *stub = nullptr;
};
