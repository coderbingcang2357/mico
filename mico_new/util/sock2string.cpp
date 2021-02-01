#include <arpa/inet.h>
#include <string.h>
#include <assert.h>
#include "util/util.h"
#include "sock2string.h"
#include <event2/util.h>

sockaddrwrap::sockaddrwrap()
{
    memset(&m_addr, 0, sizeof(m_addr));
}
void sockaddrwrap::setAddress(const sockaddr *addr, int len)
{
    memcpy(&m_addr, addr, len);
}

sockaddr *sockaddrwrap::getAddress() const
{
    return (sockaddr *)&m_addr;
}

int sockaddrwrap::getAddressLen() const
{
    if (m_addr.in.sa_family == AF_INET)
    {
        return sizeof(m_addr.in4);
    }
    else if (m_addr.in.sa_family == AF_INET6)
    {
        return sizeof(m_addr.in6);
    }
    else
    {
        return 0;
    }
}

std::string sockaddrwrap::toString() const
{
    std::string res;
    if (m_addr.in.sa_family == AF_INET)
    {
        sockaddr_in *inaddr  = (sockaddr_in *)(&m_addr.in4);
        char buf[100];
        inet_ntop(AF_INET, &(inaddr->sin_addr), buf, sizeof(buf));
        res.append(buf);
        res.append(":");
        res.append(std::to_string(inaddr->sin_port));
    }
    else if (m_addr.in.sa_family == AF_INET6)
    {
        sockaddr_in6 *inaddr  = (sockaddr_in6 *)(&m_addr.in6);
        char buf[100];
        inet_ntop(AF_INET6, &(inaddr->sin6_addr), buf, sizeof(buf));
        res.append("[");
        res.append(buf);
        res.append("]");
        res.append(":");
        res.append(std::to_string(inaddr->sin6_port));
    }
    return res;
}

uint32_t sockaddrwrap::ipv4() const
{
    if (m_addr.in.sa_family == AF_INET)
    {
        return m_addr.in4.sin_addr.s_addr;
    }
    else
    {
        return 0;
    }
}

uint16_t sockaddrwrap::port() const
{

    if (m_addr.in.sa_family == AF_INET)
    {
        return m_addr.in4.sin_port;
    }
    else if (m_addr.in.sa_family == AF_INET6)
    {
        return m_addr.in6.sin6_port;
    }
    else
    {
        return 0;
    }
}

bool sockaddrwrap::fromString(const std::string &addr)
{
    int len = sizeof(m_addr);
    int res = evutil_parse_sockaddr_port(addr.c_str(), (sockaddr *)&m_addr, &len);
    if (res == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool sockaddrwrap::operator ==(const sockaddrwrap &l)
{
    if (this->m_addr.in.sa_family != l.m_addr.in.sa_family)
        return false;
    if (m_addr.in.sa_family == AF_INET)
    {
        return m_addr.in4.sin_port == l.m_addr.in4.sin_port
                && m_addr.in4.sin_addr.s_addr == l.m_addr.in4.sin_addr.s_addr;
    }
    else if (m_addr.in.sa_family == AF_INET6)
    {
        const uint32_t *p = m_addr.in6.sin6_addr.s6_addr32;
        const uint32_t *p2 = l.m_addr.in6.sin6_addr.s6_addr32;
        bool isaddreq = true;
        for (int i = 0; i < 4; ++i)
        {
            if (*p != *p2)
            {
                isaddreq = false;
                break;
            }
        }
        return isaddreq && m_addr.in6.sin6_port == l.m_addr.in6.sin6_port;
    }
    else
    {
        return false;
    }
}

std::string sock2string(const sockaddr_in &addr)
{
    std::string result;
    char buf[1024] = {0};
    inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf));
    result.append(buf);
    result.append(":");
    result.append(std::to_string(ntohs(addr.sin_port)));
    return result;
}

sockaddr_in string2sockaddress(const std::string &ip)
{
    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    std::vector<std::string> res;
    splitString(ip, ":", &res);
    if (res.size() == 2)
    {
        inet_pton(AF_INET, res[0].c_str(), &address.sin_addr);
        address.sin_port = htons(StrtoU16(res[1]));
    }
    return address;
}

