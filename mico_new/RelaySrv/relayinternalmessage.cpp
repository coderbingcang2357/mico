#include "relayinternalmessage.h"
#include "Message/Message.h"

RelayInterMessage::RelayInterMessage(RelayInterMessage::MessagegType t)
    : m_msgtype(t)
{
}

RelayInterMessage::~RelayInterMessage(){}

RelayInterMessage::MessagegType RelayInterMessage::type()
{
    return m_msgtype;
}

// -------
MessageFromOtherServer::MessageFromOtherServer(Message *msg)
    : RelayInterMessage(RelayInterMessage::NewMessageFromOtherServer),
    m_msg(msg)
{
}

MessageFromOtherServer::~MessageFromOtherServer()
{
    delete m_msg;
}

Message *MessageFromOtherServer::getMsg()
{
    return m_msg;
}

MessageReleasePort::MessageReleasePort(uint64_t userid, uint64_t deviceid, uint64_t chid)
    : RelayInterMessage(RelayInterMessage::ReleasePort),
    m_userID(userid), m_deviceID(deviceid), m_channelid(chid)
{
}

MessageOpenChannel::MessageOpenChannel(const std::shared_ptr<ChannelInfo> &i)
    : RelayInterMessage(RelayInterMessage::OpenChannel),
    m_ci(i)
{

}

MessageCloseChannel::MessageCloseChannel(uint64_t userid, uint64_t devid)
    : RelayInterMessage(RelayInterMessage::CloseChannel),
    m_userid(userid), m_devid(devid)
{
}


MessageChannelTimeout::MessageChannelTimeout(uint64_t userid, uint64_t devid)
    : RelayInterMessage(RelayInterMessage::ChannelTimeOut),
    m_userid(userid), m_devid(devid)
{
}

MessageRemoveChannelFromDatabase::MessageRemoveChannelFromDatabase
    (uint64_t userid, uint64_t devid) : RelayInterMessage(RemoveChannelFromDatabase),
    m_userid(userid), m_deviceid(devid)
{
}

uint64_t MessageRemoveChannelFromDatabase::getUserID()
{
    return this->m_userid;
}

uint64_t MessageRemoveChannelFromDatabase::getDeviceID()
{
    return this->m_deviceid;
}

MessageInsertChannelToDatabase::MessageInsertChannelToDatabase()
    : RelayInterMessage(InsertChannelToDatabase)
{
}

