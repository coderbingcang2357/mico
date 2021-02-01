#ifndef SESSIONRANDOMNUMBER__H
#define SESSIONRANDOMNUMBER__H
#include <stdint.h>
 
void getNodeId(int port, uint32_t *nodeid);
void initialRandomnumberGenerator(uint32_t serverid);
uint64_t genSessionRandomNumber();
uint64_t getSessionRandomNumber(char *p, int plen);

#endif
