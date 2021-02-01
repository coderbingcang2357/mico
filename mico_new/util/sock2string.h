#ifndef SOCK2STRING__H__
#define SOCK2STRING__H__

#include <string>
#include <netinet/in.h>

struct sockaddrwrap
{
    sockaddrwrap();
    void setAddress(const sockaddr *addr, int m_len);
    sockaddr *getAddress() const;
    int getAddressLen() const;
    std::string toString() const;
    uint32_t ipv4() const;
    uint16_t port() const;
    bool fromString(const std::string &addr);
    bool operator == (const sockaddrwrap &l);

private:
    union {
    sockaddr in;
    sockaddr_in in4;
    sockaddr_in6 in6;
    }m_addr;
};

std::string sock2string(const sockaddr *&addr, int len);

sockaddr_in string2sockaddress(const std::string &ip);

#endif
