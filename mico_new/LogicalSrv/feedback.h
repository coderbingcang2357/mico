#ifndef FEED_BACK__H
#define FEED_BACK__H

#include "imessageprocessor.h"
class ICache;
class UserOperator;

class SaveFeedBack : public IMessageProcessor
{
public:
    SaveFeedBack(ICache *c, UserOperator *uop);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

#endif
