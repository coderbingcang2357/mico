#include "isock.h"
#include <map>

thread_local std::map<int, ISock *> sock;
ISock *ISock::get(int connid)
{
    auto it  = sock.find(connid);
    if (it != sock.end())
        return it->second;
    else
        return nullptr;
}

void ISock::set(int connid, ISock *s)
{
    auto it  = sock.find(connid);
    if (it != sock.end())
        delete it->second;
    sock[connid] = s;
}

void ISock::del()
{
    for (auto &v : sock)
    {
        delete v.second;
    }
    sock.clear();
}

ISock::ISock()
{

}

ISock::~ISock(){}
