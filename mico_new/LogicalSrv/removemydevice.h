#ifndef REMOVEMYDEVICE_H
#define REMOVEMYDEVICE_H
#include "imessageprocessor.h"
class UserOperator;
class ICache;
class RemoveMyDevice : public IMessageProcessor
{
public:
    RemoveMyDevice(ICache *ic, UserOperator *op);
    int processMesage(Message *reqmsg, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_op;
};

#endif // REMOVEMYDEVICE_H
