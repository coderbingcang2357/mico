#ifndef MESSAGE_H
#define MESSAGE_H

#include <string.h>
#include <vector>
#include <netinet/in.h>
#include <string>
//#include "../../../protocoldef/protocol.h"
#include "protocoldef/protocol.h"
//#include "Config/MsgConfig.h"
#include "itobytearray.h"
#include "util/sock2string.h"

//const uint8_t  CLIENT_MSG_PREFIX = 0xF0;  //消息开始标识
//const uint8_t  CLIENT_MSG_SUFFIX = 0xF1;  //消息结束标识
//const uint8_t  DEVICE_MSG_PREFIX = 0xE0;  //消息开始标识
//const uint8_t  DEVICE_MSG_SUFFIX = 0xE1;  //消息结束标识
//const uint16_t MAX_MSG_SIZE      = 1024; 



class Message
{
public:
    static Message *createClientMessage();
    static Message *createDeviceMessage();

    Message();

    ~Message();

    void Reset();
    void CopyHeader(const Message* msg);

    void Encrypt(char* key, int len = 16);
    int Decrypt(char* key);

    sockaddrwrap &sockerAddress()      { return m_sockAddr; }
    //sockaddr_in6& sockerAddress6()      { return m_sockAddr6; }
    uint8_t      prefix()        { return m_prefix; }
    uint16_t     serialNumber()  { return m_serialNumber; }
    uint16_t     commandID()     { return m_commandID; }
    uint16_t     versionNumber() { return m_versionNumber;}
    uint64_t     objectID()      { return m_objectID; }
    uint64_t     destID()        { return m_destID; }
    uint32_t     contentLen()    { return m_content.size();}//m_contentLen; }
    char*        content()       { return m_content.size() > 0 ? &m_content[0] : NULL ; }//m_buf + OFFSET_CONTENT; }
    uint8_t      suffix()        { return m_suffix; }

    void setSockerAddress(const sockaddrwrap &sockAddr);
    void setSockerAddress(struct sockaddr *sockAddr, int addrlen);
    void setPrefix(uint8_t prefix);
    void setSerialNumber(uint16_t serialNumber);
    void setCommandID(uint16_t cmdID);
    void setVersionNumber(uint16_t versionNumber);
    void setObjectID(uint64_t objectID);
    void setDestID(uint64_t destID);
    void setSuffix(uint8_t suffix);

    void clearContent();
    void appendContent(const void* data, uint16_t dataLen);
    void appendContent(const std::string &data = "");
    void appendContent(uint64_t d);
    void appendContent(uint32_t d);
    void appendContent(uint16_t d);
    void appendContent(uint8_t d);
    void appendContent(IToByteArray *);

    bool unpack(const char *buf, int buflen);
    void pack(std::vector<char> *out);
    void toByteArray(std::vector<char> *out);
    bool fromByteArray(const char *buf, int buflen);


    enum FieldSize
    {
        SIZE_PREFIX         = sizeof(uint8_t),
        SIZE_SERIAL_NUMBER  = sizeof(uint16_t),
        SIZE_COMMAND_ID     = sizeof(uint16_t),
        SIZE_VERSION_NUMBER = sizeof(uint16_t),
        SIZE_OBJECT_ID      = sizeof(uint64_t),
        SIZE_DEST_ID        = sizeof(uint64_t),
        SIZE_CONTENT_LEN    = sizeof(uint16_t),
        SIZE_SUFFIX         = sizeof(uint8_t),

        SIZE_HEADER_AND_ENDING = 
            + SIZE_PREFIX
            + SIZE_SERIAL_NUMBER
            + SIZE_COMMAND_ID
            + SIZE_VERSION_NUMBER
            + SIZE_OBJECT_ID
            + SIZE_DEST_ID
            + SIZE_CONTENT_LEN
            + SIZE_SUFFIX,

        SIZE_ANSWER_CODE    = sizeof(uint8_t),
        SIZE_NOTIF_NUMBER  = sizeof(uint32_t),
        SIZE_NOTIF_TYPE    = sizeof(uint16_t)
    };

    uint64_t connectionId() const;
    void setConnectionId(const uint64_t &connectionId);

    uint32_t connectionServerid() const;
    void setConnectionServerid(const uint32_t &connectionServerid);

private:
    std::vector<char> m_content;

    // connection info
    //union{
    //    struct sockaddr_in m_sockAddr;
    //struct sockaddr_in6 m_sockAddr6;
    //};
    sockaddrwrap m_sockAddr;
    uint64_t m_connectionId = 0; // 0: means udp, others means tcp connection
    uint32_t m_connectionServerid = 0; // the server id of client's tcp conntion

    uint8_t  m_prefix;
    uint16_t m_serialNumber;
    uint16_t m_commandID;
    uint16_t m_versionNumber;
    uint64_t m_objectID;
    uint64_t m_destID;
    uint8_t  m_suffix;
};


class InternalMessage : public IToByteArray
{
public:
    enum InternalMessageType
    {
        Unused = 0,
        FromExtserver = 1,
        FromRelayServer,
        FromNotifyServer,
        FromAnotherServer,
        JustForward,       // 此消息直接转发到外面
        ToNotify, // 发到通知进程去
        ToRelay, // 发到转发进程去
        ToExtServer, // 发到外面...
        ToServer, // 消息是服务器产生, 发到另一个服务器的
        Internal
    };

    InternalMessage(Message *);
    InternalMessage(Message *, uint16_t type, uint16_t subtype);
    ~InternalMessage();

    void toByteArray(std::vector<char> *out);
    bool fromByteArray(const char *buf, int buflen);
    // 1 加密, 0不加密
    bool shouldEncrypt();
    void setEncrypt(bool t);

    Message *message() {return m_msg; }
    void setType(uint16_t type) { m_type = type;}
    void setSubType(uint16_t subtype) {m_subtype = subtype;}
    uint16_t type() {return m_type; }
    uint16_t subtype() {return m_subtype; }
private:
    uint16_t m_version;
    uint16_t m_type;
    uint16_t m_subtype;
    uint8_t  m_encryptMethod;
    Message *m_msg;
};

#endif
