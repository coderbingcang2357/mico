#ifndef DBPROXYCLIENTPOOL__H
#define DBPROXYCLIENTPOOL__H

#include <memory>
#include <list>
#include <string>
#include "thread/mutexwrap.h"

class DbproxyClient;

class DbproxyClientPool
{
public:
    DbproxyClientPool(const std::string &addr, int port);
    std::shared_ptr<DbproxyClient> get();
    int poolsize();

private:
    std::string m_serveraddr;
    int m_port;

    MutexWrap m_clientslock;
    std::list<DbproxyClient *> m_clients;
};

#endif
