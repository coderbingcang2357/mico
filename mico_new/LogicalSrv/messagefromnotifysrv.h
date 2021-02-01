#ifndef MESSAGEFROMNOTIFYSRV__H
#define MESSAGEFROMNOTIFYSRV__H

#include "imessageprocessor.h"

class Notification;
class SendNotify : public IMessageProcessor
{
public:
    SendNotify();
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
   int deviceTransferNotify(Notification *notify, std::list<InternalMessage *> *r);
    int userloginAtAnotherClientNotify(Notification *notify, std::list<InternalMessage *> *r);
};

#endif
