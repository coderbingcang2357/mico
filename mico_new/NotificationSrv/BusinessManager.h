#ifndef BUSINESS_MANAGER_H
#define BUSINESS_MANAGER_H

#include <stdint.h>
#include "notifyManager.h"
#include "notifytimeout.h"
#include "Message/MsgWriter.h"

class Message;
class IDBoperator;
class NotifReqHandler;
class BusinessManager
{
public:
    BusinessManager();
    ~BusinessManager();

    int HandleIpcRequest(Message *notifReq);
    void notifyTimeout(uint64_t notifyid, int times);

private:
    void writeMessage(const Message &msg);

    NotifyManager m_notifymanager;
    NotifyTimeOut m_notifytimeout;
    IDBoperator *m_dboperator;
    NotifReqHandler *m_reqhandler;
    MsgWriterPosixMsgQueue m_resWriter;
};


#endif
