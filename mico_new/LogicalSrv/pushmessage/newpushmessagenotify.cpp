#include <assert.h>
#include "newpushmessagenotify.h"
#include "Message/Message.h"
#include "util/logwriter.h"
#include "util/util.h"
#include "pushservice.h"
#include "../server.h"
#include "../onlineinfo/iclient.h"

NewPushMessageNotify::NewPushMessageNotify(MicoServer *s, PushService *ps)
    : m_server(s), m_pushservice(ps)
{
}

int NewPushMessageNotify::processMesage(Message *msg,
    std::list<InternalMessage *> *r)
{
    char *p = msg->content();
    uint32_t len = msg->contentLen();
    if (len % 8 != 0)
    {
        ::writelog(WarningType, "error message from micom");
        return 0;
    }
    if (len == 0)
    {
        ::writelog(WarningType, "error empty message from micom");
        return 0;
    }

    auto onlineclient = m_server->onlineCache()->getAll();
    std::list<uint64_t> onlineuserid;
    for (auto &v : onlineclient)
    {
        if (GetIDType(v->id()) == IT_User)
        {
            onlineuserid.push_back(v->id());
        }
    }
    for (;len > 0;)
    {
        uint64_t pushmessageid = ::ReadUint64(p);
        p += sizeof(pushmessageid);
        len -= sizeof(pushmessageid);

        ::writelog(InfoType, "new notify from micom:%lu", pushmessageid);

        r->splice(r->end(),
            m_pushservice->getPushMessage(onlineuserid, pushmessageid));
    }

    return r->size();
}

