#ifndef __NEWPUSHMESSAGENOTIFY__H
#define __NEWPUSHMESSAGENOTIFY__H

#include "../imessageprocessor.h"

class PushService;
class MicoServer;
class NewPushMessageNotify : public IMessageProcessor
{
public:
    NewPushMessageNotify(MicoServer *s, PushService *ps);
    int processMesage(Message *, std::list<InternalMessage *> *r);

private:
    MicoServer *m_server;
    PushService *m_pushservice;
};
#endif
