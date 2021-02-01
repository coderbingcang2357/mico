#include "./realmessagerepeatechecker.h"

RealMessageRepeateChecker::RealMessageRepeateChecker(IMessageRepeatChecker *ck,
        std::set<uint16_t> *commandShouldCheck)
    : m_udpconn(ck)
{
    m_commandShouldCheck.swap(*commandShouldCheck);
}

RealMessageRepeateChecker::~RealMessageRepeateChecker()
{
    delete m_udpconn;
}

bool RealMessageRepeateChecker::isMessageRepeat(sockaddr *addr,
                uint64_t clientid,
                uint16_t msgSerialNumber,
                uint16_t commandid) 
{
    if (shouldCheck(commandid))
    {
        return m_udpconn->isMessageRepeat(addr, clientid, msgSerialNumber, commandid);
    }
    else
    {
        return false;
    }
}

void RealMessageRepeateChecker::createSession(sockaddr *addr, uint64_t clientid) 
{
    (void)addr;
    (void)clientid;
    return;
    //m_udpconn->createSession(addr, clientid);
}

void RealMessageRepeateChecker::removeSession(sockaddr *addr, uint64_t clientid) 
{
    (void)addr;
    (void)clientid;
    return;
    //m_udpconn->removeSession(addr, clientid);
}

bool RealMessageRepeateChecker::shouldCheck(uint16_t commandid)
{
    return m_commandShouldCheck.find(commandid) != m_commandShouldCheck.end();
}
