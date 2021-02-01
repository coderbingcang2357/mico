#ifndef GETSERVERLIST__H
#define GETSERVERLIST__H

#include "imessageprocessor.h"

class GetServerList : public IMessageProcessor
{
public:
    int processMesage(Message *, std::list<InternalMessage *> *r);
};

#endif
