#ifndef TCPCLIENTMESSAGE__H_
#define TCPCLIENTMESSAGE__H_
#include <stdint.h>

class TcpClientMessage
{
public:
    TcpClientMessage();
    // return len parsed
    bool fromBtyeArray(char *d, int len, int *lenparsed);

    uint16_t cmd() const;
    void setCmd(const uint16_t &cmd);

    uint16_t datalen() const;
    void setDatalen(const uint16_t &datalen);

    char *data() const;
    void setData(char *data);

    uint64_t destId() const;

    uint16_t ser() const;

private:
    uint16_t m_cmd = 0;
    uint16_t m_ser = 0;
    uint64_t m_destId = 0;
    uint16_t m_datalen = 0;
    char *m_data = nullptr;
};
#endif
