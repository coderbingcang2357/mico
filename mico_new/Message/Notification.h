#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <stdint.h>
#include <time.h>
#include <vector>
#include "Message/itobytearray.h"

class NotifIpcPacket;
class Notification : public IToByteArray
{
public:
    Notification();
    ~Notification();

    void Init(uint64_t objectID, uint8_t as, uint16_t notifType, uint64_t sn, uint16_t msgLen, char* msgBuf);

    uint8_t answercode() const {return m_answercode; }
    void setAnswcCode(uint8_t c) { m_answercode = c; }

    uint64_t serialNumber() const {return m_serialNumber; }
    void setSerialNumber(uint64_t sn) { m_serialNumber = sn; }

    time_t GetCreateTime() const { return m_createTime; }
    void SetCreateTime(time_t createTime) { m_createTime = createTime; }

    uint64_t GetObjectID() const {return m_objectID; }
    void SetObjectID(uint64_t objectID) { m_objectID = objectID; }

    uint16_t getNotifyType() const { return m_notifyType; }
    void setNotifyType(uint16_t notifNumber) { m_notifyType = notifNumber; }

    uint16_t GetMsgLen() const { return m_msgLen; }
    const char* GetMsgBuf() const ;
    void SetMsg(char* msgBuf, uint16_t msgLen);

    bool fromByteArray(char *d, int len);
    void toByteArray(std::vector<char> *out);

private:
    uint8_t m_answercode;
    uint64_t m_objectID;
    uint16_t m_notifyType;
    uint64_t m_serialNumber;
    uint16_t m_msgLen;
    std::vector<char> m_msgBuf;

    time_t m_createTime;
};


#endif
