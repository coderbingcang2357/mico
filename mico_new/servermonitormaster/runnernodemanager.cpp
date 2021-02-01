#include "runnernodemanager.h"
#include <time.h>
#include "util/logwriter.h"

static const int NodeTimeout = 60;
RunnernodeManager::RunnernodeManager()
{
}

void RunnernodeManager::registerNode(const RunnerNode &node)
{
    std::lock_guard<std::mutex> g(lock);
    auto it = m_nodesidx.find(node.nodeid);
    // already
    if (it != m_nodesidx.end())
    {
        // replace node info
        *(it->second) = node;
        it->second->status = RunnerNode::Normal;
        return;
    }
    // new
    auto newnode = std::make_shared<RunnerNode>(node);
    newnode->status = RunnerNode::Normal;
    m_nodesidx[node.nodeid] = newnode;
    m_nodes.push_back(newnode);
}

void RunnernodeManager::unregisterNode(const RunnerNode &node)
{
    std::lock_guard<std::mutex> g(lock);
    auto it = m_nodesidx.find(node.nodeid);
    if (it != m_nodesidx.end())
    {
        it->second->status = RunnerNode::Deleted; // 反正要删除，多余？？
        m_nodesidx.erase(it);
    }
}

int RunnernodeManager::nodeHeartbeat(const RunnerNode &node)
{
    std::lock_guard<std::mutex> g(lock);
    auto it = m_nodesidx.find(node.nodeid);
    if (it == m_nodesidx.end())
    {
        ::writelog(InfoType, "node heart beat can't find node : %d, %s", node.nodeid, node.ip.c_str());
        return -1;
    }
    else
    {
        it->second->hearttime = time(nullptr);
        it->second->status = RunnerNode::Normal;
        return 0;
    }
}

int RunnernodeManager::getAvailableNode(RunnerNode *rn)
{
    std::lock_guard<std::mutex> g(lock);
    // remove deleted node
    for (auto it = m_nodes.begin(); it != m_nodes.end(); )
    {
        //
        if ((*it)->status == RunnerNode::Deleted)
        {
        //
            it = m_nodes.erase(it);
        }
        else
        {
            break;
        }
    }
    //
    if (m_nodes.size() == 0)
    {
        return -1;
    }
    else
    {
        // ok,负载均衡
        *rn = **(m_nodes.begin());                
        // move firt element to last                
        auto node = *m_nodes.begin();
        m_nodes.erase(m_nodes.begin());
        m_nodes.push_back(node);
        return 0;
    }
}

int RunnernodeManager::findNode(int nodeid, RunnerNode *rn)
{
    std::lock_guard<std::mutex> g(lock);
    auto it = m_nodesidx.find(nodeid);
    if (it == m_nodesidx.end())
        return -1;

    *rn = *(it->second);
    return 0;
}

// todo : maybe change to a more eff alogrith
void RunnernodeManager::pollTimeoutNode()
{
    std::lock_guard<std::mutex> g(lock);
    time_t current = time(nullptr);
    for (auto it = m_nodes.begin(); it != m_nodes.end(); )
    {
        if ((*it)->status == RunnerNode::Deleted)
        {
            it = m_nodes.erase(it);
            continue;
        }
        else
        {
            if (current - (*it)->hearttime >= NodeTimeout)
            {
                // timeout
                (*it)->status = RunnerNode::Timeout;
            }
            ++it;
        }
    }
}
