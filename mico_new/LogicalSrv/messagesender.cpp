#include <memory>
#include "messagesender.h"
#include "onlineinfo/icache.h"
#include "Message/Message.h"
#include "onlineinfo/iclient.h"
#include "Message/MsgWriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include "util/logwriter.h"
#include "onlineinfo/rediscache.h"
#include "onlineinfo/iclient.h"
#include "tcpserver/tcpserver.h"
#include "config/configreader.h"
#include "serverinfo/serverinfo.h"
#include "serverinfo/serverid.h"
#include "udpserver.h"

MessageSender::MessageSender(Tcpserver *s, ICache *c)
    : m_tcpserver(s), m_onlineinfo(c)
{
    //Configure *cfg = Configure::getConfig();
    //std::shared_ptr<ServerInfo> server = ServerInfo::getServerInfo(cfg->serverid);

    //char tmp[50];
    //sprintf(tmp, "%s:%d", server->ip().c_str(), server->portlogical());
    //std::string ipport(tmp);
    //m_localipport.swap(ipport);
}

// 消息接收者在本机直接发
// 不在本机, 通过redis查找用户所在的电脑,发到那台电脑上面
bool MessageSender::sendMessage(InternalMessage *msg)//,
                                //sockaddr *clientaddr,
                                //int clientaddrlen,
                                //int connserverid, // which server the tcp connect to
                                //uint64_t connid) // tcp  connid, 0 means udp
{
    uint64_t recverObject = msg->message()->destID();

    if (isServerID(recverObject))
    {
        ::writelog(InfoType, 
                "receive obj is server.%u, cmd: %lu",
                recverObject, msg->message()->commandID());
    }

    if (msg->type() == InternalMessage::ToNotify)
    {
        MsgWriterPosixMsgQueue toNotify(msgqueueLogicalToNotifySrv());
        toNotify.Write(msg->message());
        //::writelog("To notify");
        assert(!msg->shouldEncrypt());
        return true;
    }
    else if (msg->type() == InternalMessage::ToRelay)
    {
        MsgWriterPosixMsgQueue toRelay(msgqueueLogicalToRelaySrv());
        toRelay.Write(msg->message());
        //::writelog("To Relay");
        assert(!msg->shouldEncrypt());
        return true;
    }
    else if (msg->type() == InternalMessage::JustForward)
    {
        assert(false);
        std::shared_ptr<IClient>  c =
            static_pointer_cast<IClient>(m_onlineinfo->getData(recverObject));
        if (c)
        {
            if (msg->shouldEncrypt())
            {
                std::vector<char> key = c->sessionKey();
                msg->message()->Encrypt(&key[0], key.size());
                msg->message()->setSockerAddress(c->sockAddr());
            }

            std::vector<char> d;
            d.reserve(512);
            msg->message()->pack(&d);
            m_tcpserver->send(c->connectionServerId(),
                              c->connectionId(),
                              c->sockAddr(),
                              //clientaddr,
                              //clientaddrlen,
                              d.data(),
                              d.size());
            return true;
        }
        else
        {
            ::writelog(InfoType,
                    "forward message can not find obj:%lu",
                    recverObject);
            return false;
        }
        //MsgWriterPosixMsgQueue toExtserver(msgqueueLogicalToExtserver());
        // toExtserver.Write(msg->message());
        //m_udp->send(msg->message());
    }
    else if (msg->type() == InternalMessage::ToServer)
    {
        return this->sendMessageToAnotherServer(msg);
    }

    // 此消息不需要处理转发, 而且不需要加密
    if (recverObject == 0)
    {
        assert(!msg->shouldEncrypt());
        //::writelog("recver obj is 0");
        //MsgWriterPosixMsgQueue toExtserver(msgqueueLogicalToExtserver());
        //toExtserver.Write(msg->message());
        //m_udp->send(msg->message());
        std::vector<char> d;
        d.reserve(512);
        msg->message()->pack(&d);
        m_tcpserver->send(msg->message()->connectionServerid(),//connserverid,
                          msg->message()->connectionId(),//connid,
                          msg->message()->sockerAddress(),
                          //clientaddr,
                          //clientaddrlen,
                          d.data(),
                          d.size());
        return true;
    }
    //else if (isServerID(recverObject))
    //{
    //    msg->setType(InternalMessage::JustForward);
    //    return this->sendMessageToAnotherServer(msg);
    //}
    // 接收者在这台电脑
    else //if (m_onlineinfo->objectExist(recverObject))
    {
        std::shared_ptr<IClient>  c;
        c = static_pointer_cast<IClient>(m_onlineinfo->getData(recverObject));
        if (!c)
        {
            RedisCache *rc = RedisCache::instance();
            c = rc->getData(recverObject);
        }
        if (c)
        {
            if (msg->shouldEncrypt())
            {
                std::vector<char> key = c->sessionKey();
                msg->message()->Encrypt(&key[0], key.size());
            }
            std::vector<char> d;
            d.reserve(512);
            msg->message()->pack(&d);
            //sockaddrwrap addr = c->sockAddr();
            //int addrlen = sizeof(addr);
            m_tcpserver->send(c->connectionServerId(),
                              c->connectionId(),
                              c->sockAddr(),
                              //(sockaddr *)&addr,
                              //addrlen,
                              d.data(), d.size());
            return true;
        }
        else
        {

            return false;
        }
        //msg->message()->setSockerAddress(c->sockAddr());
        //MsgWriterPosixMsgQueue toExtserver(msgqueueLogicalToExtserver());
        // toExtserver.Write(msg->message());
        //m_udp->send(msg->message());
    }
    // 不在本地
    //else
    //{
    //    if (a)
    //    {
            //*ipport = a->loginServerIpAndPort();
            //uint32_t serverid = a->loginServerID();
            //std::shared_ptr<ServerInfo> s = ServerInfo::getServerInfo(serverid);
            //if (!s)
            //{
            //    ::writelog(InfoType, "cannot find server id: %d", serverid);
            //    return false;
            //}
            //// *ipport = s->ipport();
            //*ipport = s->ip() + ":" + std::to_string(s->portlogical());
            //return true;
    //    }
    //    else
    //    {
    //        return false;
    //    }
        // 接收者在哪台电脑上?
        //std::string ipport;
        //if (getReceiverLoginServer(recverObject, &ipport))
        //{
        //    // 如果找的ip和端口号就是这台电脑的...但是它实际上又不在这台电脑上
        //    // 那这是什么情况呢????????
        //    if (ipport == m_localipport)
        //    {
        //        // 没找到, 丢弃此消息?
        //        ::writelog(InfoType,
        //         "message recviver not online:%lu",
        //         recverObject);
        //        return false;
        //    }
        //    else
        //    {
        //        // 找到了
        //        msg->setType(InternalMessage::JustForward);
        //        sendMessageToServer(ipport, msg);
        //        return true;
        //    }
        //}
        //else
        //{
        //    // 没找到, 丢弃此消息?
        //    ::writelog(InfoType,
        //        "message recviver not online:%lu",
        //        recverObject);
        //    return false;
        //}
    //}
}

bool MessageSender::getReceiverLoginServer(
    uint64_t o, std::string *ipport)
{
    RedisCache *rc = RedisCache::instance();
    std::shared_ptr<IClient> a = rc->getData(o);
    if (a)
    {
        //*ipport = a->loginServerIpAndPort();
        uint32_t serverid = a->loginServerID();
        std::shared_ptr<ServerInfo> s = ServerInfo::getLogicalServerInfo(serverid);
        if (!s)
        {
            ::writelog(InfoType, "cannot find server id: %d", serverid);
            return false;
        }
        // *ipport = s->ipport();
        *ipport = s->ip() + ":" + std::to_string(s->getPort());
        return true;
    }
    else
    {
        return false;
    }
}

void MessageSender::sendMessageToServer(
    const std::string &ipport, InternalMessage *msg)
{
    std::vector<char> ba;
    msg->toByteArray(&ba);
    ::writelog(InfoType, "send messge to another server: %s", ipport.c_str());
    // 告诉接收服务器帮我把这个消息发出去.
    //m_tcpserver->send(ipport, &ba[0], ba.size());
    m_tcpserver->send(ipport, ba);
}

// 把消息发到另外一台服务器上去处理
bool MessageSender::sendMessageToAnotherServer(InternalMessage *msg)
{
    uint64_t destid = msg->message()->destID();
    //if (destid == 0 || destid > 10000)
    if (!isServerID(destid))
    {
        return false;
    }
    uint64_t serverid = destid;
    std::shared_ptr<ServerInfo> si = ServerInfo::getLogicalServerInfo(serverid);
    if (!si)
    {
        ::writelog(InfoType, "can not find server of id: %d", serverid);
        return false;
    }
    //std::vector<char> ba;
    //msg->toByteArray(&ba);
    std::string ipport = si->ip() + ":" + std::to_string(si->getPort());
    this->sendMessageToServer(ipport, msg);

    return true;
}

