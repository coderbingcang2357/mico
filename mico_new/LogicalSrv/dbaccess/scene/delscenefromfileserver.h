#ifndef DELSCENEFROMFILESERVER__H
#define DELSCENEFROMFILESERVER__H
#include "idelscenefromfileserver.h"
class ISceneDal;
class DelSceneFromFileServer : public IDelScenefromFileServer
{
public:
    DelSceneFromFileServer(ISceneDal *sd);
    int postDeleteMsg(uint64_t sceneid, uint64_t userid);

private:
    ISceneDal *m_scenedal;
};
#endif
