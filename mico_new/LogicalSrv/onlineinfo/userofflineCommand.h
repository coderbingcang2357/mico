#ifndef USEROFFLINECOMMAND__CPP
#define USEROFFLINECOMMAND__CPP

#include <stdint.h>
#include "icommand.h"
class UserOfflineCommand : public ICommand
{
public:
    UserOfflineCommand(uint64_t userid)
            : ICommand(UnUsed), m_userid(userid) {}
    void execute(void *p);

private:
    uint64_t m_userid;
};
#endif
