#include <assert.h>
#include "tcpservermessage.h"
#include "protocoldef/protocol.h"
#include "util/util.h"
#include <string.h>

//        1           1    4            8      2      n     4     n    1
// SERVER_MSG_PREFIX cmd connserverid connid addrlen addr msglen msg SERVER_MSG_SUFFIX
// cmd: 1, set connserverid
//      2, new message
bool TcpServerMessage::fromByteArray(char *d, int len, int *lenparsed)
{
    int lenparsedtmp = 0;
    // skip error data
    for (;uint8_t(*d) != SERVER_MSG_PREFIX && len > 0;)
    {
        ++d;
        ++lenparsedtmp;
        --len;
        //assert(false);
    }

    if (len < 1 + 1 + 4 + 8 + 2 + 4 + 1)
    {
        *lenparsed = lenparsedtmp;
        return false;
    }

    this->cmd = ReadUint8(d + 1);
    this->connServerId = ReadUint32(d + 1 + 1);
    this->connid = ReadUint64(d + 1 + 1 + 4);
    this->addrlen = ReadUint64(d + 1 + 1 + 4 + 8);
    this->clientaddr = (sockaddr *)(d + 1 + 1 + 4 + 8 + 2);

    int packlen = 1 + 1 + 4 + 8 + 2 + this->addrlen + 4 + 1;
    if (len < packlen)
    {
        *lenparsed = lenparsedtmp;
        return false;
    }
    this->clientmessageLength = ReadUint32(d + 1 + 1 + 4 + 8 + 2 + this->addrlen);
    this->clientmessage = d + 1 + 1 + 4 + 8 + 2 + this->addrlen + 4;

    char *suffix = this->clientmessage + clientmessageLength;
    packlen = packlen + clientmessageLength;
    if (len >= packlen && uint8_t(*suffix) == SERVER_MSG_SUFFIX)
    {
        *lenparsed = lenparsedtmp + packlen;
        return true;
    }
    else
    {
        *lenparsed = lenparsedtmp;
        return false;
    }
}

//        1           1    4            8      2      n     4     n    1
// SERVER_MSG_PREFIX cmd connserverid connid addrlen addr msglen msg SERVER_MSG_SUFFIX
std::vector<char> TcpServerMessage::toByteArray()
{
    std::vector<char> res;
    res.push_back(uint8_t(SERVER_MSG_PREFIX));
    res.push_back(uint8_t(this->cmd));
    WriteUint32(&res, this->connServerId);
    WriteUint64(&res, this->connid);
    // addrlen
    ::WriteUint16(&res, this->addrlen);
    // addr
    char *ptraddr = (char *)(this->clientaddr);
    res.insert(res.end(), ptraddr, ptraddr + this->addrlen);
    // msglen
    ::WriteUint32(&res, this->clientmessageLength);
    // msg
    res.insert(res.end(), this->clientmessage, this->clientmessage + this->clientmessageLength);
    res.push_back(uint8_t(SERVER_MSG_SUFFIX));

    return res;
}

