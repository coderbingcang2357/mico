#ifndef ELETENOTIFYMESSAGE__H
#define ELETENOTIFYMESSAGE__H
#include "imessageprocessor.h"
class ICache;
class PushService;
class DeleteNotify : public IMessageProcessor
{
public:
    DeleteNotify(ICache *c, PushService *ps);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    bool decryptMessage(Message *);

    ICache *m_cache;
    PushService *m_pushservice;
};

#endif
