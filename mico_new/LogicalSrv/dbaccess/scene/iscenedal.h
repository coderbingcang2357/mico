#ifndef ISCENEDAL__H
#define ISCENEDAL__H
#include <stdint.h>
class ISceneDal
{
public:
    virtual ~ISceneDal(){}
    // 可以被共离的最大次次, 文档上写的32
    virtual int getMaxShareTimes() = 0;
    // 取一个场景被共享了多少次
    virtual int getSharedTimes(uint64_t sceneid) = 0;
    virtual int getSceneServerid(uint64_t sceneid, uint32_t *serverid) = 0;
};
#endif
