#ifndef IS_SERVERID_H
#define IS_SERVERID_H
#include <stdint.h>

uint32_t minServerID();
uint32_t maxServerID();
bool isServerID(uint64_t id);
#endif
