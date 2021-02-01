#ifndef IMESSAGESENDER__H
#define IMESSAGESENDER__H

class InternalMessage;
struct sockaddr;
class IMessageSender
{
public:
    virtual ~IMessageSender(){}
    virtual bool sendMessage(InternalMessage *msg) = 0;//,
                     //sockaddr *clientaddr,
                     //int clientaddrlen,
                     //int connserverid, uint64_t connid) = 0;
    virtual bool sendMessageToAnotherServer(InternalMessage *msg) = 0;
};
#endif
