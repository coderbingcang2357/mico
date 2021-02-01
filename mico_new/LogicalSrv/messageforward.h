#ifndef MESSAGEENCRYPT__H
#define MESSAGEENCRYPT__H
#include "tcpserver/tcpmessageprocessor.h"
#include "onlineinfo/icommand.h"

class ForwardMessage;
class InternalMessage;
// 接收到一个转发消息
class NewTcpmessageCommand : public ICommand
{
public:
    NewTcpmessageCommand(InternalMessage *);
    void execute(void *p);
    InternalMessage *message() {return m_msg; }
private:
    InternalMessage *m_msg;
};

// 收到转发消息时的处理类
class Messageforward : public Tcpmessageprocessor
{
public:
    Messageforward();
    void processMessage(TcpServerMessage *msg);

private:
};

#endif
