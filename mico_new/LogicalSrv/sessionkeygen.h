#ifndef SESSIONKEYGEN__H
#define SESSIONKEYGEN__H

#include <stdint.h>
#include <vector>
#include <string>

void genSessionKey(char *buf, int buflen);
void genSessionKey(std::vector<char> *outsessionkey);
void genLoginkey(char *buf, int buflen);
uint64_t getRandomNumber();
uint64_t genGUID();
#endif
