#include <iostream>
#include <time.h>
#include "usertimeoutcommand.h"
#include "icache.h"
#include "userdata.h"
#include "heartbeattimer.h"
#include "rediscache.h"
#include "util/logwriter.h"
#include "../gennotifyuseronlinestatus.h"
#include "protocoldef/protocol.h"
#include "Message/Message.h"
#include "mainthreadmsgqueue.h"
#include "../newmessagecommand.h"
#include "../server.h"
#include "../useroperator.h"

UserTimeoutCommand::UserTimeoutCommand(uint64_t id)
    : ICommand(UserTimeout), m_userid(id)
{
}

void UserTimeoutCommand::execute(MicoServer *server)
{
    ICache *ol = server->onlineCache();

    shared_ptr<UserData> u = static_pointer_cast<UserData>(ol->getData(m_userid));

    if (!u)
        return;

    time_t now = time(NULL);
    time_t old = u->lastHeartBeatTime();
    double elaptime = difftime(now, old);
    //std::cout << "timeout: " << elaptime << " "
    //        << u->account.c_str()
    //        << " " << u->userid
    //        << std::endl;
    // 用户心跳超时, 设置用户为下线状态, 并从redis中删除
    if (elaptime >= (60 * 2 + 30)) // 2分半
    {
        //  remove udp connection
        //server->removeUdpConnection(u->sockAddr(), m_userid);

        ol->removeObject(m_userid);
        ::unregisterTimer(m_userid);
        //u->setOnlineStatus(UserData::UserOffLine);
        // 更新u在Cache中的数据
        //ol->insertObject(m_userid, u);
        //ol->removeObject(m_userid);

        ::writelog(InfoType, "user offline %lu", m_userid);
        RedisCache::instance()->removeObject(m_userid);

        // 生成用户下线通知
        uint16_t cmdID = CMD::INCMD_USER_OFFLINE;
        Message *msg = new Message;
        msg->setSockerAddress(u->sockAddr());//后续可能有用
        msg->setCommandID(cmdID);
        msg->setObjectID(m_userid);
        InternalMessage *imsg
            = new InternalMessage(msg,
                InternalMessage::Internal,
                InternalMessage::Unused);

        MainQueue::postCommand(new NewMessageCommand(imsg));

        // 更新在线时长
        int onlinetime = time(0) - u->loginTime();
        server->asnycWork([this, server, onlinetime](){
            server->userOperator()->updateUserOnlineTime(this->m_userid,
                onlinetime);
            });
    }
}

