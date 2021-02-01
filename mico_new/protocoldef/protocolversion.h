#ifndef PROTOCOLVERSION__H
#define PROTOCOLVERSION__H
#include <stdint.h>
#include <string>

uint16_t getProtoVersion(uint16_t v);
uint16_t getClientType(uint16_t v);
std::string getClientTypeString(uint16_t clienttype);
#endif

