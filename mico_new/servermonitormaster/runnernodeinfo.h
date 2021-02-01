#pragma once
#include <string>
#include <stdint.h>
class RunnerNode
{
public:
    static const int Normal = 0;
    static const int Deleted = 1;
    static const int Timeout= 2;
    int nodeid = 0;
    std::string ip;
    int port = 0;
    uint64_t hearttime = 0;
    int status = 0;
};
