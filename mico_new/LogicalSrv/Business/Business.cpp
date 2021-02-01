#include <stdio.h>
#include <unistd.h>
#include <cassert>
#include <sys/stat.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "Business.h"
#include "util/util.h"
#include "Crypt/tea.h"
#include "Crypt/MD5.h"
#include "Message/Message.h"
#include "../onlineinfo/icache.h"
#include "../onlineinfo/userdata.h"
#include "../onlineinfo/devicedata.h"
#include "../onlineinfo/mainthreadmsgqueue.h"
#include "util/logwriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include "../imessageprocessor.h"
#include "../newmessagecommand.h"
#include "../messageforward.h"
#include "../imessagesender.h"
#include "config/configreader.h"
#include "serverinfo/serverid.h"
#include "../server.h"

InternalMessageProcessor::InternalMessageProcessor(MicoServer *server)
    : m_server(server)
{
    m_cache = server->onlineCache();
    m_tcpserver = server->tcpServer();
    m_messageSender = server->messageSender();
}

InternalMessageProcessor::~InternalMessageProcessor()
{

}

bool InternalMessageProcessor::appendMessageProcessor(uint16_t cmd,
    const std::shared_ptr<IMessageProcessor> &p)
{
    m_messageprocessor.insert(std::make_pair(cmd, p));
    return true;
}

std::shared_ptr<IMessageProcessor> InternalMessageProcessor::getMessageProcessor(uint16_t cmd)
{
    auto it = m_messageprocessor.find(cmd);
    if (it != m_messageprocessor.end())
    {
        return it->second;
    }
    else
    {
        return 0;
    }
}

// 处理线程消息
int InternalMessageProcessor::handleMessage()
{
    //if (MainQueue::hasCommand())
    {
        // std::list<InternalMessage *> msg;

        ICommand *cmd = MainQueue::getCommand();
        if (cmd == 0)
        {
            return 2;
        }

        switch (cmd->commandType())
        {
        case ICommand::UserTimeout:
            cmd->execute(this->m_server);//this->m_cache);
            delete cmd;
        break;

        case ICommand::DeviceTimeout:
            cmd->execute(this->m_server);
            delete cmd;
        break;

        case ICommand::UnUsed:
            cmd->execute(0); // 参数 0 没有用到
            delete cmd;

            break;

        case ICommand::NewMessage:
        {
            NewMessageCommand *c = dynamic_cast<NewMessageCommand *>(cmd);
            if (c)
            {
                this->processInternalMessage(c->message());
                delete c;
            }
        }
        break;

        default:
            assert(false);
        }
        return 0;
    }
    //else
    //    return 1;
}

void InternalMessageProcessor::processInternalMessage(InternalMessage *imsg)
{
    Message *msg = imsg->message();
    uint64_t destid = msg->destID();
    uint16_t type = imsg->type();
#ifndef __TEST__
#if 0
    ::writelog(InfoType,
        "new message:type: %u, srcid: %lu, destid: %lu, command: %x, len:%d", imsg->type(),
        msg->objectID(), msg->destID(), msg->commandID(), msg->contentLen());
#endif
#endif
    // 意思是直接把这个消息通过这台服务器发出去
    if (type == InternalMessage::JustForward)
    {
        //sockaddrwrap &addr = msg->sockerAddress();
        m_messageSender->sendMessage(imsg);//,
                                     //(sockaddr *)&addr,
                                     //sizeof(addr),
                                     //msg->connectionServerid(),
                                     //msg->connectionId());
    }
    else if (type == InternalMessage::Internal)
    {
        this->processMessage(msg);
    }
    // 此消息是否是由另一台电脑发过来, 要这台电脑处理的消息
    else if (type == InternalMessage::FromAnotherServer)
    {
        this->processMessage(msg);
    }
    else if (type == InternalMessage::ToServer)
    {
        Configure *c = Configure::getConfig();
        assert(msg->destID() == uint64_t(c->serverid));(void)c;
        this->processMessage(msg);
    }
    else
    {
        Configure *c = Configure::getConfig();
        // 此消息是否是要发到另一台电脑上的
        bool isTothisServer;
        //if (destid != 0 && destid < 10000)
        if (isServerID(destid))
        {
            // 本机
            if (destid == uint64_t(c->serverid))
            {
                isTothisServer = true;
            }
            else
            {
                 isTothisServer = false;
            }
        }
        else
        {
            isTothisServer = true;
        }
        if (isTothisServer) // 是发到本电脑上的
        {
            this->processMessage(msg);
        }
        else
        // destid是接收的服务器的ID, 把它发到那里去
        {
            // 这个type表示, 告诉接收的服务器这个消息是从另一个服务器发来的, 并且由
            // 接收的服务器处理这个消息
            imsg->setType(InternalMessage::FromAnotherServer);
            m_messageSender->sendMessageToAnotherServer(imsg);
        }
    }
}

void InternalMessageProcessor::processMessage(Message *msg)
{
    std::shared_ptr<IMessageProcessor> msgproc = this->getMessageProcessor(
        msg->commandID());

    uint64_t connid = msg->connectionId();
    int connserverid = msg->connectionServerid();
    sockaddrwrap addr = msg->sockerAddress();

    if (msgproc != 0)
    {
        std::list<InternalMessage *> result;
        msgproc->processMesage(msg, &result);
        for (auto it = result.begin(); it != result.end(); ++it)
        {
            Message *msg = (*it)->message();
            if (msg->destID() == 0 && msg->commandID() == CMD::CMD_GEN__NOTIFY)
            {
                // 通知的接收者是0, 这种情况connid和connserverid,addr会在
                // Sendnotify类里面设置好
                assert(msg->connectionServerid() != 0
                        && msg->sockerAddress().getAddressLen() != 0);
            }
            else
            {
                msg->setConnectionId(connid);
                msg->setConnectionServerid(connserverid);
                msg->setSockerAddress(addr);
            }
            m_messageSender->sendMessage(*it);//,
                                         //(sockaddr *)&addr,
                                         //addr.getAddressLen(),
                                         //connserverid,
                                         //connid);
            delete *it;
        }
    }
    else
    {
        ::writelog(InfoType, "unknow command id: %x", msg->commandID());
    }
}
