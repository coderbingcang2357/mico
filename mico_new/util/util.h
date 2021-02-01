#ifndef UTIL_H
#define UTIL_H

#include <mysql/mysql.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <typeinfo>
#include <time.h>
#include <vector>
#include <list>

using std::string;
typedef unsigned char byte;

const string type_U8(typeid(uint8_t).name());
const string type_U16(typeid(uint16_t).name());
const string type_U32(typeid(uint32_t).name());
const string type_U64(typeid(uint64_t).name());
const string type_Uptr(typeid(uintptr_t).name());

template<typename T>
string Uint2Str(T tUint)
{    
    string str = "";
    string type = typeid(T).name();
    if(type != type_U8 
    && type != type_U16 
    && type != type_U32 
    && type != type_U64)
        return str;   
    
    if(tUint == 0)
        return string("0");    
    while(tUint > 0) {
        str.insert(str.begin(), char(tUint % 10 + '0'));
        tUint /= 10;
    }

    return str;
}

template<typename T>
int Str2Uint(const string &str, T& outUint)
{
    if(str.length() > 20)
        return -1;

    outUint = 0;
    for(size_t i = 0; i != str.length(); i++)
    {
        if(str[i] > '9' || str[i] < '0')
        {
            return -1;
        }

        outUint = 10 * outUint + ((uint8_t) (str[i] - '0'));
    }

    return 0;
}

string GetString(char* cstr);
string BytesToStr(const byte *bytes);
void BytesToStr(const byte* in, char* out);

std::string byteArrayToHex(const uint8_t *in, int inlen);
bool hexToBytesArray(const std::string &str, std::vector<char> *out);

string U8toStr(uint8_t u8);
uint8_t StrtoU8(string str);
string U16toStr(uint16_t u16);
uint16_t StrtoU16(string str);
string U32toStr(uint32_t u32);
uint32_t StrtoU32(string str);
string U64toStr(uint64_t u64);
uint64_t StrtoU64(const string &str);

string TimetoStr(time_t t);
time_t StrtoTime(string str);

uint8_t ReadUint8(const char* pos);
uint16_t ReadUint16(const char* pos);
uint32_t ReadUint32(const char* pos);

uint64_t ReadMacAddr(const char* pos);
void uint64ToMacAddr(uint64_t, char *out);

uint64_t ReadUint64(const char* pos);
uintptr_t ReadUintptr(const char* pos);
time_t  ReadTime(const char* pos);

void WriteUint8(char* pos, uint8_t u8);
void WriteUint8(std::vector<char> *out, uint8_t value);

void WriteUint16(char* pos, uint16_t u16);
void WriteUint16(std::vector<char> *out, uint16_t value);

void WriteUint32(char* pos, uint32_t u32);
void WriteUint32(std::vector<char> *out, uint32_t value);

void WriteUint64(char* pos, uint64_t u64);
void WriteUint64(std::vector<char> *out, uint64_t value);

void WriteString(std::vector<char> *out, const std::string &str);

void WriteUintptr(char* pos, uintptr_t uptr);
void WriteTime(char* pos, time_t tm);

#define EndianSwap16(A) ((((uint16_t)(A)&0xFF00)>>8)|\
                         (((uint16_t)(A)&0x00FF)<<8))
                        
#define EndianSwap32(A) ((((uint32_t)(A)&0xFF000000)>>24)|\
                         (((uint32_t)(A)&0x00FF0000)>>8)|\
                         (((uint32_t)(A)&0x0000FF00)<<8)|\
                         (((uint32_t)(A)&0x000000FF)<<24))
                        
#define EndianSwap64(A) ((((uint64_t)(A)&0xFF00000000000000)>>56)|\
                         (((uint64_t)(A)&0x00FF000000000000)>>40)|\
                         (((uint64_t)(A)&0x0000FF0000000000)>>24)|\
                         (((uint64_t)(A)&0x000000FF00000000)>>8)|\
                         (((uint64_t)(A)&0x00000000FF000000)<<8)|\
                         (((uint64_t)(A)&0x0000000000FF0000)<<24)|\
                         (((uint64_t)(A)&0x000000000000FF00)<<40)|\
                         (((uint64_t)(A)&0x00000000000000FF)<<56))                        
            
bool IsBigEndian();            
            
//uint16_t hton16(uint16_t h16);
//uint32_t hton32(uint32_t h32);
//uint64_t hton64(uint64_t h64);
//
//uint16_t ntoh16(uint16_t n16);
//uint32_t ntoh32(uint32_t n32);
//uint64_t ntoh64(uint64_t n64);


void strncopyn(char *dest, int destlen, const char *src, int srclen);
// return bytes used
int readString(const char *d, int len, std::string *out);
int readString(const char *d, int len, std::vector<char> *out);

// string..

void splitString(const std::string &str, const std::string &sp, std::vector<string> *strlist);

// time_t
std::string timet2String(time_t t);

namespace Util1 {
bool readUint8(char **buf, int *len, uint8_t *v);
bool readUint16(char **buf, int *len, uint16_t *v);
bool readUint32(char **buf, int *len, uint32_t *v);
bool readUint64(char **buf, int *len, uint64_t *v);
bool readString(char **buf, int *len, std::string *v);
}

#endif
