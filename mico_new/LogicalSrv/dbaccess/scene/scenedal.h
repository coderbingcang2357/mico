#ifndef SCENEDAL_H__
#define SCENEDAL_H__
#include <stdint.h>
#include "iscenedal.h"
class IMysqlConnPool;
class SceneDal : public ISceneDal
{
public:
    SceneDal(IMysqlConnPool *p);
    // 可以被共离的最大次次, 文档上写的32
    int getMaxShareTimes() override;
    // 取一个场景被共享了多少次
    int getSharedTimes(uint64_t sceneid) override;
    int getSceneServerid(uint64_t sceneid, uint32_t *serverid) override;

private:
    IMysqlConnPool *m_connpool;
};
#endif

