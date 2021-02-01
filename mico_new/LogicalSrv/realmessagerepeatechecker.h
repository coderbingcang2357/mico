#ifndef REALMESSAGEREPEATECHECKER__H
#define REALMESSAGEREPEATECHECKER__H
#include "../udpsession/imessagerepeatchecker.h"
#include <set>

class RealMessageRepeateChecker : public IMessageRepeatChecker
{
public:
    RealMessageRepeateChecker(IMessageRepeatChecker *ck,
        std::set<uint16_t> *commandShouldCheck);
    ~RealMessageRepeateChecker();
    bool isMessageRepeat(sockaddr *addr,
                    uint64_t clientid,
                    uint16_t msgSerialNumber,
                    uint16_t commandid) override ;
    void createSession(sockaddr *addr, uint64_t clientid) override;
    void removeSession(sockaddr *addr, uint64_t clientid) override;

private:
    bool shouldCheck(uint16_t commandid);

    std::set<uint16_t> m_commandShouldCheck;
    IMessageRepeatChecker *m_udpconn;
};
#endif
