#include "runningscenemanager.h"
#include "util/logwriter.h"

RunningSceneManager::RunningSceneManager(int serverid, const std::shared_ptr<RunningSceneDbaccess> &db)
    : m_serverid(serverid), m_db(db)
{
}

RunningSceneManager::~RunningSceneManager()
{

}

int RunningSceneManager::addScene(uint64_t sceneid)
{
    RunningSceneData rn;
    if (m_db->getRunningScene(sceneid, &rn) != 0)
    {
        ::writelog(ErrorType, "runnscene getdatafailed: %lu", sceneid);
        return -1;
    }
    // ok
    // save serverid to databse
    m_db->setServeridOfRunningScene(sceneid, m_serverid);
    std::lock_guard<std::mutex> g(lock);
    m_scenes.insert(std::make_pair(sceneid, rn));
    // create a scene runner to run it
    m_runners.insert(std::make_pair(sceneid,
                std::shared_ptr<SceneRun>(SceneRun::runScene(rn))));
    return 0;
}

int RunningSceneManager::removeScene(uint64_t sceneid)
{
    // remove from database
    if (m_db->removeRunningScene(sceneid) == -1)
        return -1;
    std::lock_guard<std::mutex> g(lock);
    m_scenes.erase(sceneid);
    // remove from scene runner
    m_runners.erase(sceneid);
    return 0;
}

int RunningSceneManager::reloadFromDatabase()
{
    std::list<RunningSceneData> rns;
    if (m_db->getRunningSceneOfServerid(m_serverid, &rns) == -1)
    {
        ::writelog(InfoType, "restore running info failed.");
        return -1;
    }
    ::writelog(InfoType, "begin restore: size: %d", rns.size());
    std::lock_guard<std::mutex> g(lock);
    //m_scenes.clear();
    //m_runners.clear();//memoryleak!!!!!!!!!!!!!!!!!!!!!!!!11
    for (auto &r : rns)
    {
        //std::lock_guard<std::mutex> g(lock);
        m_scenes.insert(std::make_pair(r.sceneid, r));
        m_runners.insert(std::make_pair(r.sceneid,
                                        std::shared_ptr<SceneRun>(SceneRun::runScene(r))));
        ::writelog(InfoType, "reload scene: %lu", r.sceneid);
    }
    return 0;
}
