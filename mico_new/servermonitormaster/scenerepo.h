#ifndef SCENEREPO_H
#define SCENEREPO_H
#include <map>
#include <mutex>
#include "runnernodemanager.h"
#include "dbaccess/runningscenedb.h"

class SceneRepo
{
public:
    SceneRepo(RunnernodeManager *r, RunningSceneDbaccess *db);
    int isRunning(uint64_t sceneid);
    int runScene(uint64_t sceneid);
    int stopScene(uint64_t sceneid);

private:
    //std::mutex lock;
    //std::map<uint64_t, int> m_sceneidRunnerserver;
    RunnernodeManager *m_nodes;
    RunningSceneDbaccess *m_db;
};

#endif // SCENEREPO_H
