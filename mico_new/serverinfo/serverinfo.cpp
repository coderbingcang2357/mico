#include <stdio.h>
#include <unordered_map>
#include <map>
#include <stdlib.h>
#include "serverinfo.h"
#include "config/configreader.h"
#include "mysqlcli/mysqlconnection.h"
#include "util/util.h"

ServerInfo::ServerInfo(int serverid,
        const std::string &ip,
        uint16_t port,
        const std::string &type, const std::string &prop)
    :   m_serverid(serverid),
        m_ip(ip),
        m_port(port),
        m_type(type),
        m_property(prop)
{
}

int ServerInfo::serverID()
{
    return m_serverid;
}

uint16_t ServerInfo::getPort() const
{
    return m_port;
}

std::string ServerInfo::getProperty() const
{
    return m_property;
}

// 
static std::unordered_map<int, std::shared_ptr<ServerInfo> > logicalserverInfo_p;
static std::map<uint32_t, std::shared_ptr<ServerInfo> > uploadserverInfo_p;
static std::list<std::shared_ptr<ServerInfo> > extserverlist;

bool ServerInfo::readServerInfo()
{
    Configure *c = Configure::getConfig();
    for (auto &svr : c->servers)
    {
        std::shared_ptr<ServerInfo> s(new ServerInfo(svr.id,
                                        svr.ip,
                                        svr.port,
                                        svr.type,
                                        svr.property));

        //serverInfo_p.insert(std::make_pair(s->serverID(), s));

        if (svr.type == "logical")
        {
            logicalserverInfo_p.insert(std::make_pair(s->serverID(), s));
        }

        else if (svr.type == "file")
        {
            uploadserverInfo_p.insert(std::make_pair(s->serverID(), s));
        }

        else if (svr.type == "conn")
        {
            extserverlist.push_back(s);
        }

        else
        {
            return false;
        }
    }

    return logicalserverInfo_p.size() != 0
            && uploadserverInfo_p.size() != 0
            && extserverlist.size() != 0;
}

std::shared_ptr<ServerInfo> ServerInfo::getLogicalServerInfo(int serverid)
{
    auto it = logicalserverInfo_p.find(serverid);
    if (it != logicalserverInfo_p.end())
        return it->second;
    else
        return std::shared_ptr<ServerInfo>(nullptr);
}

std::shared_ptr<ServerInfo> ServerInfo::getFileServerInfo(int serverid)
{
    auto it = uploadserverInfo_p.find(serverid);
    if (it != uploadserverInfo_p.end())
        return it->second;
    else
        return std::shared_ptr<ServerInfo>(nullptr);
}

const std::list<std::shared_ptr<ServerInfo>> &ServerInfo::getExtServerlist()
{
    return extserverlist;
}

const std::unordered_map<int, std::shared_ptr<ServerInfo> > &ServerInfo::getLogicalServerlist()
{
    return logicalserverInfo_p;
}
// 上传文件的服务器.
//bool UploadServerInfo::loadServerInfo()
//{
    //std::shared_ptr<ServerInfo> s(new ServerInfo(1, "127.0.0.1", 8000));
    //uploadserverInfo_p.insert(std::make_pair(s->serverID(), s));
//    return true;
//}

uint32_t UploadServerInfo::genUploadServerID()
{
    int i = rand() % uploadserverInfo_p.size();
    auto it = uploadserverInfo_p.begin();
    if (uploadserverInfo_p.size() == 1)
        return it->first;
    for ( ;i; ++it, --i)
    {
    }
    std::string prop = it->second->getProperty();
    if (prop == "ro")
        ++it;
    if (it == uploadserverInfo_p.end())
        it = uploadserverInfo_p.begin();

    return it->first;
}

std::string UploadServerInfo::getServrUrl(uint32_t id)
{
    auto it = uploadserverInfo_p.find(id);
    if (it != uploadserverInfo_p.end())
    {
        char buf[1024];
        snprintf(buf, sizeof(buf), "%s:%d",
            it->second->ip().c_str(),
            it->second->getPort());
        return std::string(buf);
        //return it->second->ipport();
    }
    else
    {
        return std::string("");
    }
}

std::string UploadServerInfo::getServrIP(uint32_t id)
{
    auto it = uploadserverInfo_p.find(id);
    if (it != uploadserverInfo_p.end())
    {
        return it->second->ip();
    }
    else
    {
        return std::string("");
    }
}

