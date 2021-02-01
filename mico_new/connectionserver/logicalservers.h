#ifndef LOGICALSERVERS_H_
#define LOGICALSERVERS_H_
#include <stdint.h>
#include <string>
#include <map>

class LogicalServer
{
public:
    uint16_t serverid;
    std::string ip;
    uint16_t port;
};

class LogicalServers
{
public:
    LogicalServers();
    ~LogicalServers();
    bool read();
    LogicalServer get(uint16_t id);
    LogicalServer randGet();
    void insert(const LogicalServer &ls);

    std::map<uint16_t, LogicalServer> getServers() const;

private:
    std::map<uint16_t, LogicalServer> m_servers;
};
#endif
