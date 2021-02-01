#include "scenerepo.h"
#include "runneraddsceneclient.h"
#include "util/logwriter.h"

SceneRepo::SceneRepo(RunnernodeManager *r, RunningSceneDbaccess *db)
    : m_nodes(r), m_db(db)
{
}

int SceneRepo::isRunning(uint64_t sceneid)
{
    ::writelog(InfoType, "isrunning: %lu", sceneid);
    return 0;
}

int SceneRepo::runScene(uint64_t sceneid)
{
    RunnerNode rn;
    ::writelog(InfoType, "runscene: %lu", sceneid);
    int r = m_nodes->getAvailableNode(&rn);
    if (r == 0)
    {
        // call rpc run scene at runnernode
        RunerAddSccneClient client(rn.ip, rn.port);
		::writelog(InfoType,"rn.ip:%s",rn.ip.c_str());
		::writelog(InfoType,"rn.port:%d",rn.port);
        if (client.addScene(sceneid) == -1)
        {
            ::writelog(ErrorType, "runscene addscene failed: %lu", sceneid);
            return -1;
        }
        // save to db 
        if (m_db->setServeridOfRunningScene(sceneid, rn.nodeid) != 0)
        {
            ::writelog(ErrorType, "start scene write node to db failed.%lu", sceneid);
        }
        ::writelog(InfoType, "runscene %lu At node: %d", sceneid, rn.nodeid);
        //std::lock_guard<std::mutex> g(lock);
        //m_sceneidRunnerserver.insert(std::make_pair(sceneid, rn.nodeid));
        return 0;
    }
    else
    {
        ::writelog(InfoType, "runscene failed: %lu", sceneid);
        return -1;
    }
}

int SceneRepo::stopScene(uint64_t sceneid)
{
    int nodeid = 0;
    // find in database
    RunningSceneData rd;
    if (m_db->getRunningScene(sceneid, &rd) == 0)
    {
        if (rd.valid())
        {
            nodeid= rd.runningserverid;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        ::writelog(InfoType, "stopscene %lu, no nodeid", sceneid);
        return -1;
    }

    RunnerNode rn;
    int r = m_nodes->findNode(nodeid, &rn);
    if (r == 0)
    {
        // call rpc stop scene run at node
        RunerAddSccneClient client(rn.ip, rn.port);
        if (client.removeScene(sceneid) != 0)
        {
            ::writelog(InfoType, "stopscene removescene failed: %lu, nodeid: %d", sceneid, nodeid);
        }
        else
        {
            ::writelog(InfoType, "stopscene removescene success: %lu, nodeid: %d", sceneid, nodeid);
        }
        return 0;
    }
    // failed
    else
    {
        // do it later
        ::writelog(InfoType, "stopscene failed node notfound: %lu, nodeid: %d", sceneid, nodeid);
        return -1;
    }
}
