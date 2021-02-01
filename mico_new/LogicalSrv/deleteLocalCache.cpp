#include <memory>
#include "deleteLocalCache.h"
#include "Message/Message.h"
#include "util/util.h"
#include "util/logwriter.h"
#include "onlineinfo/heartbeattimer.h"
#include "config/configreader.h"
#include "./loginatanotherclientnotify.h"

DeleteLocalCache::DeleteLocalCache(ICache *lc) : localcache(lc)
{
}

int DeleteLocalCache::processMesage(Message *msg,
    std::list<InternalMessage *> *r)
{
    char *p = msg->content();
    int plen =  msg->contentLen();
    uint64_t userid;
    if (plen != (sizeof(userid) + sizeof(Magic)))
    {
        ::writelog("del local cache error msg");
        return 0;
    }

    uint64_t magic = ::ReadUint64(p);
    p += sizeof(magic);

    if (magic != Magic)
    {
        ::writelog("del local cache error magic");
        return 0;
    }

    userid = ::ReadUint64(p);
    std::shared_ptr<IClient> c = localcache->getData(userid);
    if (c)
    {
        r->push_back(LoginAtAnotherClientNotifyGen::genNotify(c, userid));
    }

    ::writelog(InfoType, "delete from here %lu", userid);
    localcache->removeObject(userid);
    ::unregisterTimer(userid);
    return r->size();
}

InternalMessage *DeleteLocalCache::getDelCacheMessage(uint64_t userid, int targetserverid)
{
    Message *msg = Message::createClientMessage();
    Configure *cfg = Configure::getConfig();
    msg->setDestID(targetserverid);
    msg->setObjectID(cfg->serverid);
    msg->setCommandID(CMD::CMD_DEL_LOCAL_CACHE);
    msg->appendContent(Magic);
    msg->appendContent(userid);
    InternalMessage *imsg = new InternalMessage(msg);
    imsg->setType(InternalMessage::ToServer);
    imsg->setEncrypt(false);
    return imsg;
}

