#ifndef RUNNERNODEMANAGER_H
#define RUNNERNODEMANAGER_H
#include <map>
#include <mutex>
#include <list>
#include <memory>
#include "runnernodeinfo.h"

class RunnernodeManager
{
public:
    RunnernodeManager();
    void registerNode(const RunnerNode &node);
    void unregisterNode(const RunnerNode &node);
    int nodeHeartbeat(const RunnerNode &node);
    int getAvailableNode(RunnerNode *rn);
    int findNode(int nodeid, RunnerNode *rn);
    void pollTimeoutNode();

private:
    std::map<int, std::shared_ptr<RunnerNode>> m_nodesidx;
    std::list<std::shared_ptr<RunnerNode>> m_nodes;
    std::mutex lock;
};

#endif // RUNNERNODEMANAGER_H
