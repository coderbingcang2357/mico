#include <assert.h>
#include "ChannelManager.h"
#include "RelayReqHandler.h"
#include "msgqueue/ipcmsgqueue.h"
#include "Message/relaymessage.h"
#include "Message/Message.h"
#include "Message/relaymessage.h"
#include "Message/MsgWriter.h"
#include "msgqueue/ipcmsgqueue.h"
#include "mainqueue.h"
#include "relayinternalmessage.h"
#include "util/logwriter.h" 
#include "config/configreader.h"
#include "sessionRandomNumber.h"
#include "relayports.h"

extern bool istest;

RelayReqHandler::RelayReqHandler(RelayPorts *ports)
    : m_ports(ports)
{
}

RelayReqHandler::~RelayReqHandler()
{

}

void RelayReqHandler::handle(Message *msg)
{
    RelayMessage rmsg;
    if (!rmsg.fromByteArray(msg->content(), msg->contentLen()))
        return;
    switch (rmsg.commandcode())
    {
    case Relay::OPEN_CHANNEL:
    {
        RelayMessage response;
        onOpenChannel(rmsg, &response);
        // w.
        MsgWriterPosixMsgQueue writer(::msgqueueRelaySrvToLogical());
        //Message *resmsg = Message::createClientMessage();
        Message resmsg;
        resmsg.CopyHeader(msg);
        resmsg.setCommandID(CMD::CMD_HMI__OPEN_RELAY_CHANNELS_RESULT);
        resmsg.appendContent(&response);
        if(!istest)
        {
            writer.Write(&resmsg);
        }
    }
    break;

    case Relay::CLOSE_CHANNEL:
        onCloseChannel(rmsg);
    break;

    case Relay::CLOSE_ALL_CHANNEL:
        onCloseAllChannel(rmsg);
    break;

    default:
        ::writelog("unknown command");
        break;
    };
}

void RelayReqHandler::onOpenChannel(RelayMessage &rmsg, RelayMessage *response)
{
    response->setUserid(rmsg.userid());
    response->setCommandCode(Relay::OPEN_CHANNEL);
    sockaddrwrap &uaddr = rmsg.usersockaddr();
    response->setUserSockaddr(uaddr);
    Configure *c = Configure::getConfig();
    sockaddr_in  serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = 0;
    inet_pton(AF_INET, c->localip.c_str(), &(serveraddr.sin_addr));
    sockaddrwrap serveraddrwrap;
    serveraddrwrap.setAddress((sockaddr *)&serveraddr, sizeof(serveraddr));
    response->setRelayServeraddr(serveraddrwrap);
    const std::list<RelayDevice> &rd = rmsg.relaydevice();
    uint64_t userid = rmsg.userid();
    for (auto it = rd.begin(); it != rd.end(); ++it)
    {
        //if (it->devicesockaddr.sin_addr.s_addr != 0)
        {
            PortPair pp = m_ports->allocate(userid, it->devicdid);

            std::shared_ptr<ChannelInfo> cinfo = std::make_shared<ChannelInfo>();
            cinfo->userID = userid;
            cinfo->deviceID = it->devicdid;
            cinfo->userfd = pp.userfd;
            cinfo->devfd = pp.devicefd;
            cinfo->userport = pp.userport;
            cinfo->deviceport = pp.devport;
            cinfo->userSessionRandomNumber = genSessionRandomNumber();
            cinfo->devSessionRandomNumber = genSessionRandomNumber();
            cinfo->channelid = pp.channelid;
            gettimeofday(&(cinfo->lastCommuniteTime), 0);
            if (pp)
            {
                MainQueue::postMsg(new MessageOpenChannel(cinfo));
            }

            RelayDevice responserelay;
            responserelay.portforclient = pp.userport;
            responserelay.portfordevice = pp.devport;
            responserelay.devicesockaddr = it->devicesockaddr;
            responserelay.devicdid = it->devicdid;
            responserelay.auth = 1;
            responserelay.userrandomNumber = cinfo->userSessionRandomNumber;
            responserelay.devrandomNumber = cinfo->devSessionRandomNumber;
            response->appendRelayDevice(responserelay);
        }
    }
}

void RelayReqHandler::onCloseChannel(const RelayMessage &imsg)
{
    const std::list<RelayDevice> &rd = imsg.relaydevice();
    for (auto it = rd.begin(); it != rd.end(); ++it)
    {
        MessageCloseChannel *cc = new MessageCloseChannel(imsg.userid(),
                                                          it->devicdid);
        MainQueue::postMsg(cc);
    }
}

void RelayReqHandler::onCloseAllChannel(const RelayMessage &r)
{
    MessageCloseChannel *cc = new MessageCloseChannel(r.userid(),
                                                      0);
    MainQueue::postMsg(cc);
}

