#ifndef MESSAGE__H
#define MESSAGE__H

#include <stdint.h>

class QueueMessage
{
public:
    enum MessageType
    {
        NewNotifyRequest,
        NotifyTimeout,
        RemoveNotify
    };

    QueueMessage(int type):m_type(type){}
    virtual ~QueueMessage(){}
    virtual int type() {return m_type;}

private:
    int m_type;
};

class NotifyTimeoutMessage : public QueueMessage
{
public:
    NotifyTimeoutMessage(uint64_t notifyid, int times)
        : QueueMessage(QueueMessage::NotifyTimeout), m_notifyid(notifyid), m_times(times)
    {
    }

    int times() {return m_times;}
    uint64_t notifyID() { return m_notifyid;}

private:
    uint64_t m_notifyid;
    int m_times;
};

class Message;
class NewNotifyRequestMessage : public QueueMessage
{
public:
    NewNotifyRequestMessage(Message *);
    ~NewNotifyRequestMessage();
    Message *getReq();

private:
    Message *m_nq;
};

#endif
