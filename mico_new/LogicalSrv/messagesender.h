#ifndef MESSAGE_SENDER__H
#define MESSAGE_SENDER__H

#include <stdint.h>
#include <string>
#include "imessagesender.h"

class ICache;
class Message;
class InternalMessage;
class Tcpserver;
class Udpserver;
struct sockaddr;
class MessageSender : public IMessageSender
{
public:
    MessageSender(Tcpserver *s, ICache *c);
    bool sendMessage(InternalMessage *msg);//,
                     //sockaddr *clientaddr,
                     //int clientaddrlen,
                     //int connserverid, uint64_t connid);
    bool sendMessageToAnotherServer(InternalMessage *msg);
    //void setUdpserver(Udpserver *u) {m_udp = u;}
private:
    bool getReceiverLoginServer(uint64_t o, std::string *ipport);
    void sendMessageToServer(const std::string &ipport, InternalMessage *msg);

    Tcpserver *m_tcpserver;
    ICache *m_onlineinfo;
    //std::string m_localipport;
    //Udpserver *m_udp;
};
#endif
