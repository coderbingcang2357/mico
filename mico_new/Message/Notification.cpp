#include <string.h>
#include <time.h>
#include <assert.h>
#include "Notification.h"
//#include "../NotifIpcPacket.h"
#include "util/util.h"
#include "util/logwriter.h"

Notification::Notification()
{
    m_objectID = 0;
    m_notifyType = 0;
    m_msgLen = 0;
    m_createTime = 0;
    m_serialNumber = 0;
    m_answercode = 0;
}

Notification::~Notification()
{
}


bool Notification::fromByteArray(char *buf, int len)
{
    char *d = buf;
    if (uint64_t(len) < sizeof(m_objectID) + sizeof(m_notifyType) + sizeof(m_serialNumber) + sizeof(m_msgLen) + sizeof(time_t))
    {
        return false;
    }
    m_answercode = ::ReadUint8(d);
    d += sizeof(m_answercode);

    m_objectID = ::ReadUint64(d);
    d += sizeof(m_objectID);

    m_notifyType = ReadUint16(d);
    d += sizeof(m_notifyType);

    m_serialNumber = ReadUint64(d);;
    d += sizeof(m_serialNumber);

    m_createTime = ReadUint64(d);
    d += sizeof(m_createTime);
    assert(sizeof(m_createTime) == 8);

    m_msgLen = ReadUint16(d);
    d += sizeof(m_msgLen);

    m_msgBuf.clear();
    m_msgBuf.insert(m_msgBuf.end(), d, (char *)(buf + len));
    assert(m_msgLen == m_msgBuf.size());

    if (m_msgLen == m_msgBuf.size())
    {
        return true;
    }
    else
    {
        ::writelog("error notify ..");
        return false;
    }
}

void Notification::toByteArray(std::vector<char> *out)
{
    out->clear();
    ::WriteUint8(out, m_answercode);
    ::WriteUint64(out, m_objectID);
    ::WriteUint16(out, m_notifyType);
    ::WriteUint64(out, m_serialNumber);
    ::WriteUint64(out, m_createTime);
    ::WriteUint16(out, m_msgLen);
    out->insert(out->end(), m_msgBuf.begin(), m_msgBuf.end());
}

void Notification::Init(
    uint64_t objectID,
    uint8_t answccode,
    uint16_t notifType,
    uint64_t sn,
    uint16_t msgLen,
    char* msgBuf)
{
    m_objectID = objectID;
    m_answercode = answccode;
    m_notifyType = notifType;
    m_createTime = time(NULL);
    m_serialNumber = sn;
    SetMsg(msgBuf, msgLen);
}

const char* Notification::GetMsgBuf() const
{
    return (char *)&m_msgBuf[0];
}

void Notification::SetMsg(char* msgBuf, uint16_t msgLen)
{
    m_msgLen = msgLen;
    m_msgBuf.clear();
    if (msgBuf == 0 || msgLen == 0)
        return;
    m_msgBuf.insert(m_msgBuf.end(), msgBuf, msgBuf + msgLen);
}

