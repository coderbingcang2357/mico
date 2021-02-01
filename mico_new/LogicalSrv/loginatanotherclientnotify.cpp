#include <assert.h>
#include "protocoldef/protocol.h"
#include "./loginatanotherclientnotify.h"
#include "onlineinfo/iclient.h"
#include "Message/Notification.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"

InternalMessage *LoginAtAnotherClientNotifyGen::genNotify(
    const std::shared_ptr<IClient> &user, uint64_t userid)
{
    assert(user);
    sockaddrwrap addr = user->sockAddr();
    std::vector<char> sesskey = user->sessionKey();

    Notification notify;
    std::vector<char> buf;
    assert(sesskey.size() == 16);
    buf.insert(buf.end(), sesskey.begin(), sesskey.end());
    ::WriteUint64(&buf, userid);
    char *p = (char *)&addr;
    buf.insert(buf.end(),p, p + sizeof(addr));
    ::WriteUint32(&buf, user->connectionServerId());
    ::WriteUint64(&buf, user->connectionId());
    ::writelog(InfoType, "LoginAtAnotherClientNotifyGen: connserverid: %u, connid: %lu",
               user->connectionServerId(),
               user->connectionId());
    notify.Init(userid,
        ANSC::SUCCESS,
        CMD::NTP_USER_LOGIN_AT_ANOTHER_CLIENT,
        0, // serial number 在notifyserver生成
        buf.size(),
        &(buf[0]));

    Message *msg = new Message;
    msg->setObjectID(0);
    msg->setCommandID(CMD::CMD_NEW_SESSIONAL_NOTIFY);
    msg->appendContent(&notify);

    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToNotify);
    imsg->setEncrypt(false);

    ::writelog(InfoType, "user login at another server notify:%lu", userid);
    return imsg;
}
