#include <assert.h>
#include "util/logwriter.h"
#include "notifyManager.h"
#include "protocoldef/protocol.h"
#include "Message/Notification.h"


NotifyManager::NotifyManager()
{

}

NotifyManager::~NotifyManager()
{
    for (auto it = m_userNotify.begin(); it != m_userNotify.end(); ++it)
    {
        auto ump = it->second;
        for (auto itump = ump.begin(); itump != ump.end(); ++itump)
        {
            delete itump->second;
        }
    }
    // 因为它们保存的notification是一样的, 所以只删除一个列表另一个直接清空就可以了
    m_idNotify.clear();
    m_userNotify.clear();
}

bool NotifyManager::insertNotify(Notification *notification)
{
    //m_userNotify.
    uint64_t serialnumber = notification->serialNumber();
    uint64_t userid = notification->GetObjectID();
    auto it = this->m_userNotify.find(userid);
    if (it == m_userNotify.end())
    {
        Notifymap nm;
        nm.insert(std::make_pair(serialnumber, notification));
        m_userNotify.insert(std::make_pair(userid, nm));
    }
    else
    {
        // 不重复添加
        if ((*it).second.find(serialnumber) != (*it).second.end())
            return false;

        (*it).second.insert(std::make_pair(serialnumber, notification));
    }
    // then insert into m_idnotify in order to find by notify id
    // 不重复添加
    if (m_idNotify.find(serialnumber) != m_idNotify.end())
        return false;

    m_idNotify.insert(std::make_pair(serialnumber, userid));

    return true;
}

//void NotifyManager::insertSessonalNotify(Notification *notification)
//{
//    assert(false);
//}

bool NotifyManager::deleteNotification(uint64_t notifNumber)
{
    Notification *notif = 0;
    uint64_t userid = 0;
    // 因为会存到两个地方 m_idNotify m_usernotify, 所以要从这两个地方删除
    // 查找通知号对应的userid
    auto it = m_idNotify.find(notifNumber);
    if (it != m_idNotify.end())
    {
        userid = it->second;
        m_idNotify.erase(it);

        // 根据userid查找通知对象
        auto usermapit = m_userNotify.find(userid);
        if (usermapit != m_userNotify.end())
        {
            auto notifymapit = usermapit->second.find(notifNumber);
            if (notifymapit != usermapit->second.end())
            {
                notif = notifymapit->second;
                usermapit->second.erase(notifymapit);
            }
        }

        if (notif != 0)
        {
            delete notif;
            return true;
        }
        else
        {
            ::writelog(InfoType,
                        "delete notification can not find noftify.1: %u",
                        notifNumber);
            return false;
        }
    }
    else
    {
        ::writelog(InfoType,
                    "delete notification can not find noftify.2: %u",
                    notifNumber);
        return false;
    }
    //
}

int NotifyManager::getByUserid(uint64_t userid,
                            std::list<Notification *> *notif)
{
    auto it = m_userNotify.find(userid);
    int size = 0;
    if (it != m_userNotify.end())
    {
        Notifymap &nm = it->second;
        size = nm.size();
        auto nmit = nm.begin();
        for ( ;nmit != nm.end() ; ++nmit)
        {
            notif->push_back(nmit->second);
        }
    }
    return size;
}

Notification *NotifyManager::getByNotifyID(uint64_t notifyid)
{
    // 查找notifyid对应的用户
    uint64_t userid = 0;
    auto it = m_idNotify.find(notifyid);
    if (it != m_idNotify.end())
    {
        userid = it->second;
    }
    else
    {
        return 0;
    }
    // 根据userid查找通知对象
    auto usermapit = m_userNotify.find(userid);
    if (usermapit != m_userNotify.end())
    {
        auto notifymapit = usermapit->second.find(notifyid);
        if (notifymapit != usermapit->second.end())
        {
            return notifymapit->second;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

