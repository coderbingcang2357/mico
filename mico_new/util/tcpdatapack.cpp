#include <assert.h>
#include <string.h>
#include "tcpdatapack.h"
#include "protocoldef/protocol.h"

// return pack len, 返回打包后数据的长度
int TcpDataPack::pack(char *buf, int len, std::vector<char> *out)
{
    if (len == 0)
        return 0;
    int totallen = len + 4; // length bytes is 4 
    char *ptolen = (char *)&totallen;
    out->insert(out->end(), ptolen, ptolen + sizeof(totallen));
    out->insert(out->end(), buf, buf + len);
    assert(out->size() == static_cast<std::vector<char>::size_type>(len + 4));
    return out->size();
}

// 返回共解包了多少字节
int TcpDataPack::unpack(char *buf, int len, std::vector<char> *out)
{
    if (len < 5) // 长度4字节, 至少1字节数据, 所以一包至少5字节
        return 0;

    int packlen;
    memcpy(&packlen, buf, sizeof(packlen));

    // 超过包最大长度, 则只取最大包长度
    if (packlen > int(MAX_MSG_SIZE))
        packlen = MAX_MSG_SIZE;

    // packlen 长度至少为5
    if (packlen < 5)
    {
        packlen = 5;
    }

    if (packlen >= 5 && packlen <= len) // ok find a pack
    {
        out->resize(packlen - 4);
        memcpy(&((*out)[0]), buf + 4, packlen - 4);
        return packlen;
    }
    else
    {
        return 0;
    }
}
