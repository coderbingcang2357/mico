#include "./logicalservers.h"
#include "serverinfo/serverinfo.h"

LogicalServers::LogicalServers()
{
    //LogicalServer ls;
    //ls.ip = "10.1.240.39";
    //ls.port = 8889;
    //ls.serverid = 1;
    //this->insert(ls);
    //ls.ip = "10.1.240.44";
    //ls.serverid = 3;
    //this->insert(ls);
}

LogicalServers::~LogicalServers()
{

}

bool LogicalServers::read()
{
    //ServerInfo
    auto logicalservers = ServerInfo::getLogicalServerlist();
    for (auto &server : logicalservers )
    {
        LogicalServer ls;
        ls.serverid = server.second->serverID();
        ls.ip = server.second->ip();
        ls.port = server.second->getPort();
        this->insert(ls);
    }
    return true;
}

LogicalServer LogicalServers::get(uint16_t id)
{
    return m_servers.at(id);
}

LogicalServer LogicalServers::randGet()
{
    return m_servers.begin()->second;
}

void LogicalServers::insert(const LogicalServer &ls)
{
    m_servers.insert(std::make_pair(ls.serverid, ls));
}

std::map<uint16_t, LogicalServer> LogicalServers::getServers() const
{
    return m_servers;
}
