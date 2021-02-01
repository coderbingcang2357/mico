#ifndef NOTIF_REQ_HANDLER_H
#define NOTIF_REQ_HANDLER_H

#include <vector>
#include <stdint.h>

class Notification;
//class NotifRequest;
class NotifyManager;
class NotifyTimeOut;
class IDBoperator;
class Message;

class NotifReqHandler
{
public:
    NotifReqHandler(NotifyManager *nm, NotifyTimeOut *to, IDBoperator *);
    ~NotifReqHandler();

    std::vector<Notification*>& Execute(Message *notifReq);
    void ClearResponses();
private:

    void OnSaveOnlineNotif(Notification *notification);
    void OnSaveSessionalOnlineNotif(Notification *notification);
    void OnSaveOfflineNotif(Notification *notification);
    void OnGetNotifWhenLogin(uint64_t objectID);
    void OnGetNotifWhenBackOnline(uint64_t objectID);
    void OnDeleteNotif(uint64_t objectID, uint64_t notifNumber);
    void OnDumpNotif(uint64_t objectID);

    void OutputResponse(Notification* notifRes);
private:
    std::vector<Notification*> m_vNotifRes;
    NotifyManager *m_notifymanager;
    NotifyTimeOut *m_notifytimeout;
    IDBoperator *dbOperator;
};


#endif
