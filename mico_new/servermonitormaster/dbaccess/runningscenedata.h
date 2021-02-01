#pragma once
#include <stdint.h>
#include <string>

class RunningSceneData
{
public:
    bool valid(){return sceneid != 0;}
    uint64_t sceneid = 0;
    uint8_t  wecharten = 1;
    std::string wechartphone;
    uint8_t  msgen = 0;
    std::string msgphone;
    uint64_t startuserid = 0;
    std::string blobscenedata;
    int runningserverid = 0;
};
