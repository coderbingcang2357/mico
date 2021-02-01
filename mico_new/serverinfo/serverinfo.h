#ifndef SERVER_INFO__H
#define SERVER_INFO__H
#include <stdint.h>
#include <string>
#include <memory>
#include <list>
#include <unordered_map>

class ServerInfo
{
public:
    static bool readServerInfo();
    static std::shared_ptr<ServerInfo> getLogicalServerInfo(int serverid);
    static std::shared_ptr<ServerInfo> getFileServerInfo(int serverid);
    static const std::list<std::shared_ptr<ServerInfo>> &getExtServerlist();
    static const std::unordered_map<int, std::shared_ptr<ServerInfo> > &getLogicalServerlist();

    ServerInfo(int serverid,
        const std::string &ip,
               uint16_t port,
               const std::string &type,
               const std::string &prop);
    int serverID();
    std::string ip() {return m_ip;}
    std::string type() {return m_type;}
    uint16_t getPort() const;

    std::string getProperty() const;

private:
    int m_serverid;
    std::string m_ip;
    uint16_t  m_port;
    std::string m_type;
    std::string m_property;
};

class UploadServerInfo
{
public:
    //static bool loadServerInfo();
    static uint32_t genUploadServerID();
    static std::string getServrUrl(uint32_t id);
    static std::string getServrIP(uint32_t id);
};
#endif
