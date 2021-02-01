#pragma once
#include <iostream>
#include <assert.h>

using namespace std;

inline unsigned char ToHex(unsigned char x) 
{ 
	return  x > 9 ? x + 55 : x + 48; 
}

unsigned char FromHex(unsigned char x);
std::string UrlDecode(const std::string& str);

