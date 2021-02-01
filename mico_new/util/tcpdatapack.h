#ifndef TCPDATAPACK__H_
#define TCPDATAPACK__H_

#include <vector>
class TcpDataPack
{
public:
    static int pack(char *buf, int len, std::vector<char> *out);
    static int unpack(char *buf, int len, std::vector<char> *out);
};
#endif
