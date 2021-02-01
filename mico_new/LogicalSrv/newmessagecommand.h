#ifndef NEW_MESSAGE_COMMAND__H
#define NEW_MESSAGE_COMMAND__H
#include "onlineinfo/icommand.h"

class InternalMessage;
class NewMessageCommand : public ICommand
{
public:
    NewMessageCommand(InternalMessage *msg);
    void execute(MicoServer*) override;

    ~NewMessageCommand();
    InternalMessage *message() {return m_msg;}
private:
    InternalMessage *m_msg;
};

#endif
