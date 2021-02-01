#include <functional>
#include <assert.h>
#include "dbproxyclientpool.h"
#include "thread/mutexguard.h"
#include "dbproxyclient.h"

DbproxyClientPool::DbproxyClientPool(const std::string &addr, int port)
    : m_serveraddr(addr), m_port(port)
{
}

int DbproxyClientPool::poolsize()
{
    MutexGuard guard(&m_clientslock);
    return m_clients.size();
}

std::shared_ptr<DbproxyClient> DbproxyClientPool::get()
{
    DbproxyClient *client = 0;
    // find in list
    {
        MutexGuard guard(&m_clientslock);
        for (;m_clients.size() > 0;)
        {
            client = m_clients.back();
            m_clients.pop_back();
            if (client->ping() == DbSuccess)
            {
                break;
            }
            else
            {
                assert(false);
                delete client;
                client = 0;
            }
        }
    }

    // can not find in list so we create a new client and connect to server
    if (client == 0)
    {
        client = new DbproxyClient(m_serveraddr, m_port);
        if (client->connect() != DbSuccess)
        {
            delete client;
            client = 0;
        }
    }

    // return the result
    return std::shared_ptr<DbproxyClient>(client,
            [this](DbproxyClient *cli) {
                MutexGuard guard(&this->m_clientslock);
                this->m_clients.push_front(cli);
            }
        );
}

