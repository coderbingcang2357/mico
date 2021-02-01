#include "newmessagecommand.h"
#include "Message/Message.h"

NewMessageCommand::NewMessageCommand(InternalMessage *msg)
    : ICommand(NewMessage), m_msg(msg)
{
}

NewMessageCommand::~NewMessageCommand()
{
    delete m_msg;
}

void NewMessageCommand::execute(MicoServer *)
{
}

