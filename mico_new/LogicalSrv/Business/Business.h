#ifndef BUSINESS_H
#define BUSINESS_H
#include <set>
#include <map>
#include <memory>
#include "../onlineinfo/icache.h"

//class ExtMsgHandler;
class MessageSender;
class Tcpserver;
class InternalMessage;
class IMessageProcessor;
class Message;
class IMessageSender;
class MicoServer;

class InternalMessageProcessor
{
public:    
    InternalMessageProcessor(MicoServer *server);
    ~InternalMessageProcessor();

    // int PollOnlineCache();
    //int HandleIpcPacketFromIntConnSrv();
    //int HandleIpcPacketFromExtConnSrv();
    //int HandleIpcPacketFromNotifSrv();
    //int HandleIpcPacketFromRelaySrv();
    int handleMessage();

    ICache *cache() {return m_cache;}
    //ExtMsgHandler *m_extmsghandler;

    bool appendMessageProcessor(uint16_t cmd, const std::shared_ptr<IMessageProcessor> &p);

private:
    void processInternalMessage(InternalMessage *imsg);
    void processMessage(Message *msg);

    std::shared_ptr<IMessageProcessor> getMessageProcessor(uint16_t);
    //void processForwardMessage(InternalMessage *imsg);

    ICache *m_cache;
    MicoServer *m_server;
    Tcpserver *m_tcpserver;
    IMessageSender *m_messageSender;
    std::map<uint32_t, std::shared_ptr<IMessageProcessor> > m_messageprocessor;
};

#endif
