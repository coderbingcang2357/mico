#include <assert.h>
#include <iostream>
#include "tcpdatapack.h"

int main()
{
    char buf[10];
    std::vector<char> out;
    for (int i = 0; i < int(sizeof(buf)); i++)
        buf[i] = i;
    int len = TcpDataPack::pack(buf, sizeof(buf), &out);
    assert(len == out.size());(void)len;

    std::vector<char> outbuf;
    int unpacklen = TcpDataPack::unpack(&(out[0]), out.size(), &outbuf);
    assert(unpacklen == outbuf.size() + 4);
    for (int i = 0; i < unpacklen - 4; i++)
    {
         assert(outbuf[i] == i);
    }

    std::cout << "ok\n";
    return 0;
}
