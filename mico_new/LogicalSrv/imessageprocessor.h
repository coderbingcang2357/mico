#ifndef IMESSAGE_PROCESSOR__H
#define IMESSAGE_PROCESSOR__H

#include <list>
class Message;
class InternalMessage;
class IMessageProcessor
{
public:
    virtual ~IMessageProcessor(){}
    virtual int processMesage(Message *, std::list<InternalMessage *> *r) = 0;
};
#endif
