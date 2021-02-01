#include <assert.h>
#include <string.h>
#include "tcpclientmessage.h"
#include "protocoldef/protocol.h"
#include "util/util.h"

TcpClientMessage::TcpClientMessage()
{
}

bool TcpClientMessage::fromBtyeArray(char *d, int len, int *lenparsed)
{
    int lenparsedtmp = 0;
    for (;uint8_t(*d) != CLIENT_MSG_PREFIX
         && uint8_t(*d) != DEVICE_MSG_PREFIX && len > 0;)
    {
        ++lenparsedtmp;
        len--;
        ++d;
        //assert(false);
    }
    // ...
    if (len < 1 + 6 + 8 + 8 + 2 + 1)
    {
        *lenparsed = lenparsedtmp;
        return false;
    }

    // cmd
    this->m_ser = ::ReadUint16(d + 1);
    this->m_cmd = ::ReadUint16(d + 3);
    //
    this->m_destId = ::ReadUint64(d + 1 + 6 + 8);

    // datalen
    this->m_datalen = ::ReadUint16(d + 1 + 6 + 8 + 8);

    // data
    this->m_data = d + 1 + 6 + 8 + 8 + 2;
    //
    char *suffix = m_data + m_datalen;

    int packlen = 1 + 6 + 8 + 8 + 2 + m_datalen + 1;

    if (len < packlen)
    {
        *lenparsed = lenparsedtmp;
        return false;
    }

    if (uint8_t(*suffix) != CLIENT_MSG_SUFFIX
            && uint8_t(*suffix) != DEVICE_MSG_SUFFIX)
    {
        //assert(false);
        *lenparsed = lenparsedtmp;
        return false;
    }
    else
    {
        lenparsedtmp += packlen;
        *lenparsed = lenparsedtmp;
        return true;
    }
}

uint16_t TcpClientMessage::cmd() const
{
    return m_cmd;
}

void TcpClientMessage::setCmd(const uint16_t &cmd)
{
    m_cmd = cmd;
}

uint16_t TcpClientMessage::datalen() const
{
    return m_datalen;
}

void TcpClientMessage::setDatalen(const uint16_t &datalen)
{
    m_datalen = datalen;
}

char *TcpClientMessage::data() const
{
    return m_data;
}

void TcpClientMessage::setData(char *data)
{
    m_data = data;
}

uint64_t TcpClientMessage::destId() const
{
    return m_destId;
}

uint16_t TcpClientMessage::ser() const
{
    return m_ser;
}

