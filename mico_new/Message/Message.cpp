#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "Message.h"
//#include "MsgHandler.h"
//#include "../Util/Util.h"
#include "util/util.h"
//#include "../Crypt/tea.h"
#include "Crypt/tea.h"
//#include "../Config/Debug.h"
#include "util/logwriter.h"

//构造
Message::Message()
    : m_prefix(CLIENT_MSG_PREFIX),
     m_serialNumber(0),
     m_commandID(0),
     m_versionNumber(0),
     m_objectID(0),
     m_destID(0),
     m_suffix(CLIENT_MSG_SUFFIX)
{
    m_content.reserve(100);
}

Message *Message::createClientMessage()
{
    Message *m = new Message;
    m->m_prefix = CLIENT_MSG_PREFIX;
    m->m_suffix = CLIENT_MSG_SUFFIX;
    return m;
}

Message *Message::createDeviceMessage()
{
    Message *m = new Message;
    m->m_prefix = DEVICE_MSG_PREFIX;
    m->m_suffix = DEVICE_MSG_SUFFIX;;
    return m;
}

//析构
Message::~Message()
{
    //delete [] m_buf;
}

//重设（置零）
void Message::Reset()
{
    bzero(&m_sockAddr, sizeof(m_sockAddr));

    m_serialNumber = 0;
    m_commandID = 0;
    m_versionNumber = 0;
    m_objectID = 0;
    m_destID = 0;

    setPrefix(CLIENT_MSG_PREFIX);
    setSuffix(CLIENT_MSG_SUFFIX);

    m_content.clear();
    m_content.reserve(100);
}

void Message::pack(std::vector<char> *out)
{
    out->reserve(SIZE_HEADER_AND_ENDING + contentLen());

    char *p = (char *)&m_prefix;
    out->insert(out->end(), p, p + SIZE_PREFIX);

    p = (char *)&m_serialNumber;
    out->insert(out->end(), p, p + SIZE_SERIAL_NUMBER);

    p = (char *)&m_commandID;
    out->insert(out->end(), p, p + SIZE_COMMAND_ID);

    p = (char *)&m_versionNumber;
    out->insert(out->end(), p, p + SIZE_VERSION_NUMBER);

    p = (char *)&m_objectID;
    out->insert(out->end(), p, p + SIZE_OBJECT_ID);

    p = (char *)&m_destID;
    out->insert(out->end(), p, p + SIZE_DEST_ID);

    // content len
    uint16_t contentlen = m_content.size();
    p = (char *)&contentlen;
    out->insert(out->end(), p, p + SIZE_CONTENT_LEN);

    // content
    out->insert(out->end(), m_content.begin(), m_content.end());

    // suffix
    p = (char *)&m_suffix;
    out->insert(out->end(), p, p + SIZE_SUFFIX);
}

void Message::toByteArray(std::vector<char> *out)
{
    pack(out);
}

//解包（不包括数据域）
bool Message::unpack(const char *buf, int buflen)
{
    if (buflen < SIZE_HEADER_AND_ENDING)
    {
        return false;
    }

    const char* pos = buf;
    // m_contentLen = m_len - SIZE_HEADER_AND_ENDING;

    memcpy(&m_prefix,        pos,                        SIZE_PREFIX);
    memcpy(&m_serialNumber,  pos += SIZE_PREFIX,         SIZE_SERIAL_NUMBER);
    memcpy(&m_commandID,     pos += SIZE_SERIAL_NUMBER,  SIZE_COMMAND_ID);
    memcpy(&m_versionNumber, pos += SIZE_COMMAND_ID,     SIZE_VERSION_NUMBER);
    memcpy(&m_objectID,      pos += SIZE_VERSION_NUMBER, SIZE_OBJECT_ID);
    memcpy(&m_destID,        pos += SIZE_OBJECT_ID,      SIZE_DEST_ID);
    pos += SIZE_DEST_ID;

    // content length
    uint16_t  contentLen;
    memcpy(&contentLen,      pos, SIZE_CONTENT_LEN);
    pos += SIZE_CONTENT_LEN;

    // .
    if (pos + contentLen + SIZE_SUFFIX != buf + buflen)
    {
        ::writelog("Error Message length error.");
        //assert(false);
        return false;
    }
    // content
    std::vector<char> tmpcontent(pos, pos + contentLen);
    pos += contentLen;
    m_content.swap(tmpcontent);

    memcpy(&m_suffix, pos, SIZE_SUFFIX);

    if (m_prefix != CLIENT_MSG_PREFIX
            && m_prefix != DEVICE_MSG_PREFIX
            && m_prefix != CD_MSG_PREFIX)
    {
        ::writelog("Error Message Prefix");
        return false;
    }

    if (m_suffix != CLIENT_MSG_SUFFIX
            && m_suffix != DEVICE_MSG_SUFFIX
            && m_suffix != CD_MSG_SUFFIX)
    {
        ::writelog("Error Message Suffix");
        return false;
    }
    return true;
}

bool Message::fromByteArray(const char *buf, int buflen)
{
    return unpack(buf, buflen);
}

uint64_t Message::connectionId() const
{
    return m_connectionId;
}

void Message::setConnectionId(const uint64_t &connectionId)
{
    m_connectionId = connectionId;
}

uint32_t Message::connectionServerid() const
{
    return m_connectionServerid;
}

void Message::setConnectionServerid(const uint32_t &connectionServerid)
{
    m_connectionServerid = connectionServerid;
}

//复制（除数据域）
void Message::CopyHeader (const Message* msg)
{
    m_sockAddr = msg->m_sockAddr;
    m_prefix = msg->m_prefix;
    m_serialNumber = msg->m_serialNumber;
    m_commandID = msg->m_commandID;
    m_versionNumber = msg->m_versionNumber;
    m_objectID = msg->m_objectID;
    // m_destID = msg->m_destID;
    m_destID = 0;
    m_suffix = msg->m_suffix;
}

void Message::setSockerAddress(const sockaddrwrap &sockAddr)
{
    m_sockAddr = sockAddr;
}

void Message::setSockerAddress(sockaddr *sockAddr, int addrlen)
{
    m_sockAddr.setAddress(sockAddr, addrlen);
}

void Message::setPrefix(uint8_t prefix)
{
    m_prefix = prefix;
}

void Message::setSerialNumber(uint16_t serialNumber)
{
    m_serialNumber = serialNumber;
}

void Message::setCommandID(uint16_t commandID)
{
    m_commandID = commandID;
}

void Message::setVersionNumber(uint16_t versionNumber)
{
    m_versionNumber = versionNumber;
}

void Message::setObjectID(uint64_t objectID)
{
    m_objectID = objectID;
}

void Message::setDestID(uint64_t destID)
{
    m_destID = destID;
}

void Message::setSuffix(uint8_t suffix)
{
    m_suffix = suffix;
}

//解密数据域
int Message::Decrypt (char* key)
{
    char plainBuf[contentLen() * 2 + 18]; // ..
    int plainLen = tea_dec(content(), contentLen(), plainBuf, key, 16);
    if(plainLen < 0)
        return plainLen;

    std::vector<char> tmpcontent(plainBuf, plainBuf + plainLen);
    std::swap(m_content, tmpcontent);

    return plainLen;
}

//加密数据域
void Message::Encrypt (char* key, int keylen)
{
    char cipherBuf[contentLen() * 2 + 18];
    int cipherLen = tea_enc(content(), contentLen(), cipherBuf, key, keylen);

    std::vector<char> tmpcontent(cipherBuf, cipherBuf + cipherLen);
    std::swap(m_content, tmpcontent);

}

void Message::clearContent()
{
    m_content.clear();
}

//添加数据域项
void Message::appendContent (const void* data, uint16_t dataLen)
{
    m_content.insert(m_content.end(), (char *)data, (char *)data + dataLen);
}

void Message::appendContent(uint64_t d)
{
    char *p = (char *)&d;
    appendContent(p, sizeof(d));
}

void Message::appendContent(uint32_t d)
{
    char *p = (char *)&d;
    appendContent(p, sizeof(d));
}

void Message::appendContent(uint16_t d)
{
    char *p = (char *)&d;
    appendContent(p, sizeof(d));
}

void Message::appendContent(uint8_t d)
{
    char *p = (char *)&d;
    appendContent(p, sizeof(d));
}

//添加数据域项
void Message::appendContent(const std::string &data)
{
    if (data.length() == 0)
    {
        m_content.push_back(uint8_t(0));
    }
    else
    {
        m_content.insert(m_content.end(),
            (char *)data.c_str(), (char *)data.c_str() + data.length() + 1);
    }
}

void Message::appendContent(IToByteArray *data)
{
    std::vector<char> outdata;
    data->toByteArray(&outdata);
    this->appendContent(&outdata[0], outdata.size());
}


InternalMessage::InternalMessage(Message *msg)
    : m_version(0), m_type(0), m_subtype(0), m_msg(msg)
{
    setEncrypt(true);
}

InternalMessage::InternalMessage(Message *msg, uint16_t type, uint16_t subtype)
    : m_version(0), m_type(type), m_subtype(subtype), m_msg(msg)
{
}

InternalMessage::~InternalMessage()
{
    delete m_msg;
}

void InternalMessage::toByteArray(std::vector<char> *out)
{
    char *p = (char *)&m_version;
    out->insert(out->end(), p, p + sizeof(uint16_t));

    p = (char *)&m_type;
    out->insert(out->end(), p, p + sizeof(uint16_t));

    p = (char *)&m_subtype;
    out->insert(out->end(), p , p + sizeof(uint16_t));

    out->push_back(uint8_t(m_encryptMethod));

    std::vector<char> msgbytearray;
    m_msg->toByteArray(&msgbytearray);

    out->insert(out->end(), msgbytearray.begin(), msgbytearray.end());
}

bool InternalMessage::fromByteArray(const char *buf, int buflen)
{
    if (uint32_t(buflen) <= sizeof(m_version)
            + sizeof(m_type)
            + sizeof(m_subtype)
            + sizeof(m_encryptMethod))
    {
        return false;
    }
    const char *p = buf;
    m_version = ReadUint16(p);
    p += sizeof(uint16_t);

    m_type = ReadUint16(p);
    p += sizeof(uint16_t);

    m_subtype = ReadUint16(p);
    p += sizeof(uint16_t);

    m_encryptMethod = ReadUint8(p);
    p += sizeof(uint8_t);

    return m_msg->fromByteArray(p, buf + buflen - p);
}

// 1 加密, 0不加密
bool InternalMessage::shouldEncrypt()
{
    return m_encryptMethod != 0;
}

// 1 加密, 0不加密
void InternalMessage::setEncrypt(bool t)
{
    m_encryptMethod = t ? 1 : 0;
}

