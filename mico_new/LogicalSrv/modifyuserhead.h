#ifndef  MODIFYUSERHEAD__H
#define  MODIFYUSERHEAD__H

#include <list>
#include <stdint.h>
#include <string>
#include "imessageprocessor.h"

class UserOperator;
class ICache;
class Notification;
class TransferDeviceToAnotherCluster;
class MicoServer;
class DeviceAutho;

class ModifyUserHead : public IMessageProcessor
{
public:
    ModifyUserHead(ICache *c, UserOperator *uop);
    int processMesage(Message *reqmsg, std::list<InternalMessage *> *r);

private:
    ICache *m_cache;
    UserOperator *m_useroperator;
};

#endif
