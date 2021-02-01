#ifndef DEVICETIMEOUTCOMMAND_H
#define DEVICETIMEOUTCOMMAND_H

#include <stdint.h>
#include "icommand.h"

class DeviceTimeoutCommand : public ICommand
{
public:
    DeviceTimeoutCommand(uint64_t id);
    void execute(MicoServer *p) override;
    uint64_t id() { return m_devid; }

private:
    uint64_t m_devid;
};

#endif

