#ifndef USERHEARTBEAT__H
#define USERHEARTBEAT__H
#include <stdint.h>

void registerUserTimer(uint64_t userid);
void registerDeviceTimer(uint64_t devid);
void unregisterTimer(uint64_t id);

#endif
