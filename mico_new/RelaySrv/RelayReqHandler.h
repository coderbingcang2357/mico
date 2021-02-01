#ifndef RELAY_REQ_HANDLER_H
#define RELAY_REQ_HANDLER_H
#include <vector>
#include <stdint.h>

class Message;
class RelayMessage;
class ChannelManager;
class RelayPorts;
class RelayReqHandler
{
public:
    RelayReqHandler(RelayPorts *ports);
    ~RelayReqHandler();
    void handle(Message *msg);
    //int Execute();
private:
    void onOpenChannel(RelayMessage &, RelayMessage *response);
    void onCloseChannel(const RelayMessage &);
    void onCloseAllChannel(const RelayMessage &);

    RelayPorts *m_ports;
};


#endif
