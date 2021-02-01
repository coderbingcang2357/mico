#include <string.h>
#include "util/util.h"
#include "relaymessage.h"

RelayMessage::RelayMessage()
    : m_commandcode(0),
    m_userid(0)
{
    memset(&m_usersockaddr, 0, sizeof(m_usersockaddr));
    memset(&m_relayserveraddr, 0, sizeof(m_relayserveraddr));
}

void RelayMessage::appendRelayDevice(const RelayDevice &rd)
{
    m_relaydevice.push_back(rd);
}

void RelayMessage::toByteArray(std::vector<char> *out)
{
    ::WriteUint8(out, m_commandcode);
    ::WriteUint64(out, m_userid);

    char *p = (char *)&m_usersockaddr;
    out->insert(out->end(), p, p + sizeof(m_usersockaddr));

    p = (char *)&m_relayserveraddr;
    out->insert(out->end(), p, p + sizeof(m_relayserveraddr));

    uint16_t dcnt = m_relaydevice.size();
    ::WriteUint16(out, dcnt);

    for (auto it = m_relaydevice.begin(); it != m_relaydevice.end(); ++it)
    {
        const RelayDevice &rd = *it;
        ::WriteUint8(out, rd.auth);
        ::WriteUint64(out, rd.devicdid);
        ::WriteUint64(out, rd.userrandomNumber);
        ::WriteUint64(out, rd.devrandomNumber);
        p = (char *)&(rd.devicesockaddr);
        out->insert(out->end(), p, p + sizeof(rd.devicesockaddr));
        ::WriteUint16(out, rd.portforclient);
        ::WriteUint16(out, rd.portfordevice);
    }
}

bool RelayMessage::fromByteArray(char *d, uint32_t len)
{
    uint16_t devcnt;
    uint32_t hdr = sizeof(devcnt) + sizeof(m_commandcode) + sizeof(m_userid) + sizeof(sockaddrwrap) + sizeof(sockaddrwrap);
    if (uint32_t(len) < hdr)
    {
        return false;
    }
    m_commandcode = ReadUint8(d);
    d += sizeof(m_commandcode);

    m_userid = ReadUint64(d);
    d += sizeof(m_userid);

    //memcpy(m_usersockaddr
    memcpy(&m_usersockaddr, d, sizeof(m_usersockaddr));
    d += sizeof(m_usersockaddr);

    memcpy(&m_relayserveraddr, d, sizeof(m_relayserveraddr));
    d += sizeof(m_relayserveraddr);


    devcnt = ReadUint16(d);
    d += sizeof(devcnt);

    len -= hdr;

    if (len < devcnt * RelayDevice::RelayDeviceSize)
    {
        return false;
    }

    for(int i = 0; i < devcnt; ++i)
    {
        RelayDevice rd;

        rd.auth = ::ReadUint8(d);
        d += sizeof(rd.auth);

        rd.devicdid = ::ReadUint64(d);
        d += sizeof(rd.devicdid);

        rd.userrandomNumber = ::ReadUint64(d);
        d += sizeof(rd.userrandomNumber);

        rd.devrandomNumber = ::ReadUint64(d);
        d += sizeof(rd.devrandomNumber);

        memcpy(&rd.devicesockaddr, d, sizeof(rd.devicesockaddr));
        d += sizeof(rd.devicesockaddr);

        rd.portforclient = ::ReadUint16(d);
        d += sizeof(rd.portforclient);

        rd.portfordevice = ::ReadUint16(d);
        d += sizeof(rd.portfordevice);

        m_relaydevice.push_back(rd);
    }

    return true;
}

