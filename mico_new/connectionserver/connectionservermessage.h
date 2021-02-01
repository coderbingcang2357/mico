#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdint.h>
#include <vector>
#include <string>
#include "util/sock2string.h"
struct sockaddr;

enum MessageType
{
    NewConnectionMessageType,
    SendToClientMessageType,
    SendToServerMessageType,
    UdpSendToClientMessageType,
    UdpSendToServerMessageType,
    ClientConnectionClosedMessageType,
    ChangeConnectionIdMessageType
};

class IConnectionServerMessage
{
public:
    IConnectionServerMessage(int type);
    virtual ~IConnectionServerMessage();
    virtual int type();
private:
    int m_msgtype;
};

class NewConnectionMessage : public IConnectionServerMessage
{
public:
    NewConnectionMessage();
    ~NewConnectionMessage();
    void setAddress(sockaddr *a, int addrlen);

    int fd() const;
    void setFd(int fd);

    uint64_t connSessionId() const;
    void setConnSessionId(const uint64_t &connSessionId);

    uint32_t logicalserverid() const;
    void setLogicalserverid(const uint32_t &logicalserverid);

    sockaddrwrap &addr();

private:
    int m_fd = -1;
    uint64_t m_connSessionId = 0;
    uint32_t m_logicalserverid = 0;
    //int m_addrlen = 0;
    //sockaddr *m_addr = nullptr;
    sockaddrwrap m_addr;
};

class SendToClientMessage : public IConnectionServerMessage
{
public:
    SendToClientMessage();

    int connSessionId() const;
    void setConnSessionId(int connSessionId);

    std::vector<char> &data();

    void setData(const std::vector<char> &data);

private:
    int m_connSessionId= 0;
    std::vector<char> m_data;
};

class UdpSendToClientMessage  : public IConnectionServerMessage
{
public:
    UdpSendToClientMessage();
    ~UdpSendToClientMessage();
    void setAddr(sockaddr *addr, int len);
    sockaddrwrap &clientAddr();

    std::vector<char> &data() ;

    void setData(const std::vector<char> &data);

private:
    sockaddrwrap m_clientaddr;
    std::vector<char> m_data;
};

class SendToServerMessage : public IConnectionServerMessage
{
public:
    SendToServerMessage();

    uint32_t logicalServerid;
    uint64_t connid;
    std::vector<char> data;
};

class UdpSendToServerMessage : public IConnectionServerMessage
{
public:
    UdpSendToServerMessage();
    std::string ipport;
    uint32_t logicalserverid = 0;
    std::vector<char> data;
};

class ClientConnectionClosedMessage : public IConnectionServerMessage
{
public:
    ClientConnectionClosedMessage();

    void *slaveserver = nullptr;
    uint64_t connid = 0;
};

class ChangeConnectionIdMessage : public IConnectionServerMessage
{
public:
    ChangeConnectionIdMessage();
    uint64_t oldconnid = 0;
    uint64_t newconnid = 0;
    void *slaveserver = nullptr;
};
#endif
