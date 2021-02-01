#ifndef SEARCHUSER__H
#define SEARCHUSER__H

#include "imessageprocessor.h"
#include "onlineinfo/icache.h"

class UserOperator;
class SearchUser : public  IMessageProcessor
{
public:
    SearchUser(ICache *, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    UserOperator *m_useroperator;
    ICache *m_cache;
};
#endif
