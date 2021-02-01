#ifndef RELAYMESSAGE_H
#define RELAYMESSAGE_H


#include <stdint.h>
#include <list>
#include <netinet/in.h>
#include "itobytearray.h"
#include "util/sock2string.h"

class RelayDevice
{
public:
    static const uint32_t RelayDeviceSize
        = sizeof(uint8_t) // auth
            + sizeof(uint64_t) // devid
            + sizeof(uint64_t) // user rnd nbr
            + sizeof(uint64_t) // dev rnd nbr
            + sizeof(sockaddrwrap) // dev sock
            + sizeof(uint16_t) // port for cli
            + sizeof(uint16_t); // port for dev
    uint8_t auth;
    uint64_t devicdid;
    uint64_t userrandomNumber;
    uint64_t devrandomNumber;
    sockaddrwrap devicesockaddr;
    // 下面的这两个参数只有返回结果时才有用
    uint16_t portforclient;
    uint16_t portfordevice;
};

class RelayMessage : public IToByteArray
{
public:
    RelayMessage();
    uint8_t commandcode() const {return this->m_commandcode; }
    void setCommandCode(uint8_t c) { this->m_commandcode = c; }

    uint64_t userid() const { return m_userid; }
    void setUserid(uint64_t uid) { m_userid = uid; }

    sockaddrwrap &usersockaddr() { return m_usersockaddr; }
    void setUserSockaddr(const sockaddrwrap &addr) { m_usersockaddr = addr; }

    sockaddrwrap &relayServeraddr() { return m_relayserveraddr;};
    void setRelayServeraddr(const sockaddrwrap &addr) { m_relayserveraddr = addr; }

    const std::list<RelayDevice> &relaydevice() const { return m_relaydevice; };
    void appendRelayDevice(const RelayDevice &rd);

    void toByteArray(std::vector<char> *out);
    bool fromByteArray(char *d, uint32_t len);

private:
    uint8_t m_commandcode;
    uint64_t m_userid;
    uint64_t userRandomNumber;
    sockaddrwrap m_usersockaddr;
    sockaddrwrap m_relayserveraddr;
    std::list<RelayDevice> m_relaydevice;
};

#endif
