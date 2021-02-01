#ifndef USERTIMEOUTCOMMAND_H
#define USERTIMEOUTCOMMAND_H
#include <stdint.h>
#include "icommand.h"

class UserTimeoutCommand : public ICommand
{
public:
    UserTimeoutCommand(uint64_t id);
    void execute(MicoServer *p) override;
    uint64_t userID() { return m_userid; }

private:
    uint64_t m_userid;
};
#endif
