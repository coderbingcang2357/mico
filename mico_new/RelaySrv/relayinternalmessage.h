#ifndef RELAYINTERNALMESSAGE__H
#define RELAYINTERNALMESSAGE__H
#include <stdint.h>
#include "ChannelManager.h"
#include "util/sock2string.h"

class Message;
class RelayInterMessage 
{
public:
    enum MessagegType
    {
        NewMessageFromOtherServer = 1,
        ReleasePort,
        OpenChannel,
        CloseChannel,
        ChannelTimeOut, 
        InsertChannelToDatabase,
        RemoveChannelFromDatabase
    };

    RelayInterMessage(RelayInterMessage::MessagegType t);
    virtual ~RelayInterMessage();
    RelayInterMessage::MessagegType type();

private:
    MessagegType m_msgtype;
};

class MessageFromOtherServer : public RelayInterMessage
{
public:
    MessageFromOtherServer(Message *msg);
    ~MessageFromOtherServer();
    Message *getMsg();
private:
    Message *m_msg;
};

class MessageReleasePort : public RelayInterMessage
{
public:
    MessageReleasePort(uint64_t userid, uint64_t deviceid, uint64_t chid);
    uint64_t userID() {return m_userID;}
    uint64_t deviceID() {return m_deviceID;}
    uint64_t channelid() {return m_channelid;}
private:
    uint64_t m_userID;
    uint64_t m_deviceID;
    uint64_t m_channelid;
};

class MessageOpenChannel : public RelayInterMessage
{
public:
    MessageOpenChannel(const std::shared_ptr<ChannelInfo> &i);
    std::shared_ptr<ChannelInfo> channelinfo() { return m_ci; }

private:
    std::shared_ptr<ChannelInfo> m_ci;
};

class MessageCloseChannel : public RelayInterMessage
{
public:
    MessageCloseChannel(uint64_t userid, uint64_t devid);
    uint64_t userid() {return m_userid;}
    uint64_t devid() {return m_devid;}
private:
    uint64_t m_userid;
    uint64_t m_devid;
};

class MessageChannelTimeout : public RelayInterMessage
{
public:
    MessageChannelTimeout(uint64_t userid, uint64_t devid);
    uint64_t userid() { return m_userid; }
    uint64_t deviceid() { return m_devid; }
private:
    uint64_t m_userid;
    uint64_t m_devid;
};


class MessageRemoveChannelFromDatabase : public RelayInterMessage
{
public:
    MessageRemoveChannelFromDatabase(uint64_t userid, uint64_t devid);
    uint64_t getUserID();
    uint64_t getDeviceID();
private:
    uint64_t m_userid;
    uint64_t m_deviceid;
};

//  serverid      | int(10) unsigned    
//  userid        | bigint(20) unsigned 
//  deviceid      | bigint(20) unsigned 
//  userport      | int(10) unsigned    
//  deviceport    | int(10) unsigned    
//  userfd        | int(11)             
//  devicefd      | int(11)             
//  useraddress   | varchar(100)        
//  deviceaddress
class MessageInsertChannelToDatabase : public RelayInterMessage
{
public:
    MessageInsertChannelToDatabase();
    int &serverid() { return m_serverid; }
    uint64_t & userid() { return m_userid; }
    uint64_t & deviceid() { return m_deviceid; }
    uint16_t & userport() { return m_userport; }
    uint16_t & deviceport() { return m_deviceport; }
    int & userfd() { return m_userfd; }
    int & devicefd() { return m_devicefd; }
    sockaddrwrap & useraddress() { return m_useraddress; }
    sockaddrwrap & deviceaddress() { return m_deviceaddress; }

private:
    int m_serverid;
    uint64_t m_userid;
    uint64_t m_deviceid;
    uint16_t m_userport;
    uint16_t m_deviceport;
    int m_userfd;
    int m_devicefd;
    sockaddrwrap m_useraddress;
    sockaddrwrap m_deviceaddress;
};

#endif

