#include "gennotifyuseronlinestatus.h"
#include "Message/Message.h"
#include "useroperator.h"
#include "Message/Notification.h"
#include "util/util.h"

GenNotifyUserOnlineStatus::GenNotifyUserOnlineStatus(uint64_t userid, uint8_t onlinestatus, UserOperator *uop)
    : m_userid(userid), m_onlinestatu(onlinestatus), m_useroperator(uop)
{
}

void GenNotifyUserOnlineStatus::genNotify(std::list<InternalMessage *> *r)
{
    std::vector<uint64_t> clusteridlist;
    m_useroperator->getClusterIDListOfUser(m_userid, &clusteridlist);
    for (auto it = clusteridlist.begin(); it != clusteridlist.end(); ++it)
    {
        std::vector<char> buf;
        ::WriteUint64(&buf, *it); // 用户所在的群ID
        ::WriteUint64(&buf, m_userid);
        ::WriteUint8(&buf, m_onlinestatu);
        Notification notify;
        notify.Init(0,
            ANSC::SUCCESS,
            CMD::NTP_DEV__USER_ONLINE_STATUS_CHANGE,
            0,
            buf.size(),
            &buf[0]);

        //genNotify(it->id, imsg);
        std::list<uint64_t> idlist;
        m_useroperator->getUserIDInDeviceCluster(*it, &idlist);

        for (auto it = idlist.begin(); it != idlist.end(); ++it)
        {
            uint64_t recvuid = *it;

            if (recvuid == m_userid)
                continue;

            notify.SetObjectID(recvuid);

            Message *msg = new Message;
            msg->setObjectID(recvuid);
            msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
            msg->appendContent(&notify);

            InternalMessage *imsg = new InternalMessage(msg);
            imsg->setType(InternalMessage::ToNotify);
            imsg->setEncrypt(false);
            r->push_back(imsg);
        }
    }
}

