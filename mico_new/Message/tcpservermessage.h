#ifndef TCPSERVERMESSAGE__H_
#define TCPSERVERMESSAGE__H_
#include <stdint.h>
#include <vector>

struct sockaddr;

class TcpServerMessage
{
public:
    enum CMD:uint8_t {
        SetConnServerId = 1,
        NewMessage,
        NewUdpMessage,
        HeartBeat
    };
    // return len parsed
    bool fromByteArray(char *d, int len, int *lenparsed);
    std::vector<char> toByteArray();

    uint8_t cmd;
    uint32_t connServerId = 0;
    uint64_t connid;
    uint16_t addrlen = 0;
    sockaddr *clientaddr = nullptr;
    uint32_t clientmessageLength = 0;
    char *clientmessage = nullptr;
};
#endif
