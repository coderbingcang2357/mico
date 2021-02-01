#include <string.h>
#include "connectionservermessage.h"


IConnectionServerMessage::IConnectionServerMessage(int type) : m_msgtype(type)
{
}

IConnectionServerMessage::~IConnectionServerMessage(){}

int IConnectionServerMessage::type()
{
    return m_msgtype;
}

NewConnectionMessage::NewConnectionMessage() : IConnectionServerMessage(NewConnectionMessageType)
{

}

NewConnectionMessage::~NewConnectionMessage()
{
}

void NewConnectionMessage::setAddress(sockaddr *a, int addrlen)
{
    this->m_addr.setAddress(a, addrlen);
}

int NewConnectionMessage::fd() const
{
    return m_fd;
}

void NewConnectionMessage::setFd(int fd)
{
    m_fd = fd;
}

uint64_t NewConnectionMessage::connSessionId() const
{
    return m_connSessionId;
}

void NewConnectionMessage::setConnSessionId(const uint64_t &connSessionId)
{
    m_connSessionId = connSessionId;
}

uint32_t NewConnectionMessage::logicalserverid() const
{
    return m_logicalserverid;
}

void NewConnectionMessage::setLogicalserverid(const uint32_t &logicalserverid)
{
    m_logicalserverid = logicalserverid;
}

sockaddrwrap &NewConnectionMessage::addr()
{
    return m_addr;
}

SendToClientMessage::SendToClientMessage() : IConnectionServerMessage(SendToClientMessageType)
{

}

int SendToClientMessage::connSessionId() const
{
    return m_connSessionId;
}

void SendToClientMessage::setConnSessionId(int connSessionId)
{
    m_connSessionId = connSessionId;
}

std::vector<char> &SendToClientMessage::data()
{
    return m_data;
}

void SendToClientMessage::setData(const std::vector<char> &data)
{
    m_data = data;
}

SendToServerMessage::SendToServerMessage() : IConnectionServerMessage(SendToServerMessageType)
{

}

//
ClientConnectionClosedMessage::ClientConnectionClosedMessage()
    : IConnectionServerMessage(ClientConnectionClosedMessageType)
{

}

ChangeConnectionIdMessage::ChangeConnectionIdMessage()
    : IConnectionServerMessage(ChangeConnectionIdMessageType)
{

}

UdpSendToServerMessage::UdpSendToServerMessage()
    : IConnectionServerMessage(UdpSendToServerMessageType)
{

}

UdpSendToClientMessage::UdpSendToClientMessage()
    : IConnectionServerMessage(UdpSendToClientMessageType)
{

}

UdpSendToClientMessage::~UdpSendToClientMessage()
{
}

void UdpSendToClientMessage::setAddr(sockaddr *addr, int len)
{
    this->m_clientaddr.setAddress(addr, len);
}

sockaddrwrap &UdpSendToClientMessage::clientAddr()
{
    return m_clientaddr;
}

std::vector<char> &UdpSendToClientMessage::data()
{
    return m_data;
}

void UdpSendToClientMessage::setData(const std::vector<char> &data)
{
    m_data = data;
}
