#ifndef DEVICE_OFFLINE_COMMAND_H
#define DEVICE_OFFLINE_COMMAND_H

#include <stdint.h>
#include "icommand.h"
class DeviceData;
class DeviceOfflineCommand : public ICommand
{
public:
    DeviceOfflineCommand(uint64_t devid, int type)
            : ICommand(type), m_devid(devid) {}
    void execute(void *p);
private:
    uint64_t m_devid;
};
#endif
