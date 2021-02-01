#ifndef GETUSERINFO___H
#define GETUSERINFO___H

#include "imessageprocessor.h"

class ICache;
class UserOperator;

class GetUserInfo : public IMessageProcessor
{
public:
    GetUserInfo(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_userop;
};

#endif

