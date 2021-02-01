#pragma once
#include <vector>
#include <string>
typedef unsigned char BYTE;

std::string Base64Encode(BYTE const* buf, unsigned int bufLen);
std::vector<BYTE> Base64Decode(std::string const&);

