#include <assert.h>
#include "util/logwriter.h"
#include "messageforward.h"
#include "onlineinfo/mainthreadmsgqueue.h"
#include "forwardmessage.h"
#include "Message/Message.h"
#include "Message/tcpservermessage.h"
#include "newmessagecommand.h"

NewTcpmessageCommand::NewTcpmessageCommand(InternalMessage *m)
    : ICommand(ForwardMessage), m_msg(m)
{

}

void NewTcpmessageCommand::execute(void *p)
{

}

Messageforward::Messageforward()
{
}

void Messageforward::processMessage(TcpServerMessage *msg)
{
    InternalMessage *fmsg = new InternalMessage(new Message);
    fmsg->message()->setConnectionId(msg->connid);
    fmsg->message()->setConnectionServerid(msg->connServerId);
    fmsg->message()->setSockerAddress(msg->clientaddr, msg->addrlen);
    if (fmsg->message()->fromByteArray(msg->clientmessage, msg->clientmessageLength))
    {
        // assert(fmsg->type() > 0);
        //::writelog(InfoType, "message from another server1: %u", fmsg->message()->commandID());
		//uint64_t objectID = fmsg->message()->objectID();
        //::writelog(InfoType, "message from another server1: %llu", fmsg->message()->objectID());
        //NewTcpmessageCommand *cmd = new NewTcpmessageCommand(fmsg);
        NewMessageCommand *cmd = new NewMessageCommand(fmsg);
        MainQueue::postCommand(cmd);
        //::writelog(InfoType, "get new clientmsg");
    }
    else if (fmsg->fromByteArray(msg->clientmessage, msg->clientmessageLength))
    {
        assert(fmsg->type() > 0);
        ::writelog(InfoType, "message from another server: %u", fmsg->message()->commandID());
        //NewTcpmessageCommand *cmd = new NewTcpmessageCommand(fmsg);
        NewMessageCommand *cmd = new NewMessageCommand(fmsg);
        MainQueue::postCommand(cmd);

    }
    else
    {
        //assert(false);
        ::writelog("message forward fromByteArray failed.");
        delete fmsg;
    }
}
