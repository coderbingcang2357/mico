#include "queuemessage.h"
#include "Message/Message.h"

NewNotifyRequestMessage::NewNotifyRequestMessage(Message *nq)
        : QueueMessage(NewNotifyRequest), m_nq(nq)
{
}

NewNotifyRequestMessage::~NewNotifyRequestMessage()
{
    delete m_nq;
}

Message *NewNotifyRequestMessage::getReq()
{
    return m_nq;
}
