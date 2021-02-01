#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <stdint.h>
#include <unordered_map>
#include <map>
#include <list>

class Notification;

class NotifyManager
{
public:
    NotifyManager();
    ~NotifyManager();
    bool insertNotify(Notification *notification);
    //void insertSessonalNotify(Notification *notification);
    bool deleteNotification(uint64_t notifNumber);
    int getByUserid(uint64_t userid, std::list<Notification *> *notif);
    Notification* getByNotifyID(uint64_t notifyid);

private:
    // notifyid map notify
    typedef uint64_t NotifyID;
    typedef uint64_t ObjectID;
    typedef std::map<NotifyID, Notification *> Notifymap;
    // 通过userid找通知
    std::unordered_map<ObjectID, Notifymap> m_userNotify;
    // 通过通知号找通知
    std::map<NotifyID, ObjectID> m_idNotify;
};

#endif
