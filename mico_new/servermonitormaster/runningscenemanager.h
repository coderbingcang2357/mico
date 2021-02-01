#pragma once
#include <stdint.h>
#include <map>
#include <memory>
#include "dbaccess/runningscenedata.h"
#include "dbaccess/runningscenedb.h"
#include <mutex>
#include "scenerunner.h"

class RunningSceneManager
{
public:
    RunningSceneManager(int serverid, const std::shared_ptr<RunningSceneDbaccess> &db);
    ~RunningSceneManager();
    int addScene(uint64_t sceneid);
    int removeScene(uint64_t sceneid);
    int reloadFromDatabase();

private:
    int m_serverid = -1;
    std::mutex lock;
    std::map<uint64_t, RunningSceneData> m_scenes;
    std::map<uint64_t, std::shared_ptr<SceneRun>> m_runners;
    std::shared_ptr<RunningSceneDbaccess> m_db;
};
