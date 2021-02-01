#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "util.h"

const char HEX[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
void BytesToStr(const byte* in, char* out)
{
    for(size_t i = 0; i < 16; i++) {
        int t = in[i];
        int a = t / 16;
        int b = t % 16;

        out[i * 2] = HEX[a];
        out[i * 2 + 1] = HEX[b];
    }
}

string GetString(char* cstr)
{
    return ((cstr != NULL) ? string(cstr) : "");
}

string BytesToStr(const byte* bytes)
{
    char cstr[33] = {0};
    for(size_t i = 0; i < 16; i++) {
        int t = bytes[i];
        int a = t / 16;
        int b = t % 16;

        cstr[i * 2] = HEX[a];
        cstr[i * 2 + 1] = HEX[b];
    }
    cstr[32] = 0;
    return string(cstr);
}

std::string byteArrayToHex(const uint8_t *in, int inlen)
{
    std::string out;

    for (int i = 0; i < inlen; i++)
    {
        int t = in[i];
        out.push_back(HEX[t / 16]);
        out.push_back(HEX[t % 16]);
    }

    return out;
}

static
int hexchar2Int(char ch)
{
  if((ch >= '0') && (ch <= '9'))
    return (ch - '0');
  else if((ch >= 'a') && (ch <= 'f'))
    return (ch - 'a' + 0x0A);
  else if((ch >= 'A') && (ch <= 'F'))
    return (ch - 'A' + 0x0A);
  else 
    return -1;
}

bool hexToBytesArray(const std::string &str, std::vector<char> *out)
{
    int s = str.size();

    if (s % 2 != 0)
        return false;

    const char *p = str.c_str();

    for (int i = 0; i < s; i+=2)
    {
        out->push_back( hexchar2Int((*p)) * 16 + hexchar2Int(*(p + 1)) );
        p += 2;
    }

    return true;
}

string U64toStr(uint64_t u64)
{
    if(u64 == 0)
        return string("0");

    string str;
    while(u64 > 0) {
        str.insert(str.begin(), char(u64 % 10 + '0'));
        u64 /= 10;
    }

    return str;    
}

uint64_t StrtoU64(const string &str)
{
    uint64_t ret = 0;
    for(size_t i = 0; i != str.length(); i++) {
        if(str[i] > '9' || str[i] < '0')
            break;

        ret = 10 * ret + ((uint8_t) (str[i] - '0'));
    }

    return ret;
}

uint8_t StrtoU8(string str)
{
    return uint8_t(str[0] - '0');
}

uint16_t StrtoU16 (string str)
{
    uint16_t ret = 0;
    for(size_t i = 0; i != str.length(); i++) {
        if(str[i] > '9' || str[i] < '0')
            break;

        ret = 10 * ret + ((uint8_t) (str[i] - '0'));
    }

    return ret;
}

uint32_t StrtoU32 (string str)
{
    uint32_t ret = 0;
    for(size_t i = 0; i != str.length(); i++) {
        if(str[i] > '9' || str[i] < '0')
            break;

        ret = 10 * ret + ((uint8_t) (str[i] - '0'));
    }

    return ret;
}

//uint64_t StrtoU64(string str)
//{
//    uint64_t ret = 0;
//    for(size_t i = 0; i != str.length(); i++) {
//        if(str[i] > '9' || str[i] < '0')
//            break;
//        
//        ret = 10 * ret + ((uint8_t) (str[i] - '0'));
//    }
//
//    return ret;
//}

time_t StrtoTime(string str)
{
    time_t ret = 0;
    for(size_t i = 0; i != str.length(); i++) {
        if(str[i] > '9' || str[i] < '0')
            break;

        ret = 10 * ret + ((uint8_t) (str[i] - '0'));
    }

    return ret;    
}

string U8toStr(uint8_t u8)
{
    char str[2] = {0};
    str[0] = u8 + '0';
    return string(str);
}

string U16toStr(uint16_t u16)
{
    if(u16 == 0)
        return string("0");

    string str;
    while(u16 > 0) {
        str.insert(str.begin(), char(u16 % 10 + '0'));
        u16 /= 10;
    }

    return str;     
}

string U32toStr(uint32_t u32)
{
    if(u32 == 0)
        return string("0");

    string str;
    while(u32 > 0) {
        str.insert(str.begin(), char(u32 % 10 + '0'));
        u32 /= 10;
    }

    return str;    
}

//string U64toStr(uint64_t u64)
//{
//    if(u64 == 0)
//        return string("0");
//
//    string str;
//    while(u64 > 0) {
//        str.insert(str.begin(), char(u64 % 10 + '0'));
//        u64 /= 10;
//    }
//
//    return str;
//}

string TimetoStr(time_t t)
{
    if(t == 0)
        return string("0");

    string str;
    while(t > 0) {
        str.insert(str.begin(), char(t % 10 + '0'));
        t /= 10;
    }

    return str;    
}

uint8_t ReadUint8(const char * pos)
{
    uint8_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(uint8_t));
    
    return ret;
}

uint16_t ReadUint16(const char * pos)
{
    uint16_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(uint16_t));
    
    return ret;
}

uint32_t ReadUint32(const char* pos)
{
    uint32_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(uint32_t));
    
    return ret; 
}

uint64_t ReadMacAddr(const char* pos)
{
    uint64_t ret = 0;
    for(int i = 0; i < 6; i++)
    {
        ret = 0x100 * ret + ((uint8_t) pos[i]);
    }
    return ret;
}

// 这个函数是ReadMacAddr的逆过程
// mac 占6字节, 所以out至少要6字节
// 这里假定机器是小尾
void uint64ToMacAddr(uint64_t mac, char *out)
{
    assert(!IsBigEndian());
    char *p = (char *)&mac;
    p += 5;
    for (int i = 0; i < 6; i++)
    {
        out[i] = *p;
        p--;
    }
}

uint64_t ReadUint64(const char* pos)
{
    uint64_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(uint64_t));
    
    return ret; 
}

time_t ReadTime(const char* pos)
{
    time_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(ret));
    
    return ret; 
}

uintptr_t ReadUintptr (const char* pos)
{
    uintptr_t ret = 0;
    memcpy((void*) &ret, (void*) pos, sizeof(uintptr_t));

    return ret; 
}

//uint64_t ReadMacAddr(const char* pos)
//{
//    uint64_t ret = 0;
//    for(int i = 0; i < 6; i++)
//    {
//        ret = 0x100 * ret + ((uint8_t) pos[i]);
//    }
//    return ret;
//}

// -----------------
void WriteUint8(char* pos, uint8_t u8)
{   
    memcpy((void*) pos, (void*) &u8, sizeof(uint8_t));
}

void WriteUint8(std::vector<char> *out, uint8_t value)
{
    // should i do endian transfer???
    out->push_back(value);
}

static void writevalue_p(std::vector<char> *o, void *v, int len)
{
    char *p = (char *)v;
    o->insert(o->end(), p, p + len);
}

// ----------------
void WriteUint16(char* pos, uint16_t u16)
{   
    memcpy((void*) pos, (void*) &u16, sizeof(uint16_t));
}

void WriteUint16(std::vector<char> *out, uint16_t value)
{
    writevalue_p(out, &value, sizeof(value));
}

//-----------------
void WriteUint32(char* pos, uint32_t u32)
{
    memcpy((void*) pos, (void*) &u32, sizeof(uint32_t));
}

void WriteUint32(std::vector<char> *out, uint32_t value)
{
    writevalue_p(out, &value, sizeof(value));
}

// -------------------------
void WriteUint64(char* pos, uint64_t u64)
{   
    memcpy((void*) pos, (void*) &u64, sizeof(uint64_t));
}

void WriteUint64(std::vector<char> *out, uint64_t value)
{
    writevalue_p(out, &value, sizeof(value));
}

void WriteString(std::vector<char> *out, const std::string &str)
{
    char *p = (char *)str.c_str();
    out->insert(out->end(), p, p + str.size() + 1); // 加1是因为字符串要以0结尾
}

// -------------------------
void WriteUintptr (char* pos, uintptr_t uptr)
{
    memcpy((void*) pos, (void*) &uptr, sizeof(uintptr_t));
}

void WriteTime(char* pos, time_t time)
{
    memcpy((void*) pos, (void*) &time, sizeof(time_t));
}

bool IsBigEndian()
{
    union
    {
        uint32_t u32;
        unsigned char uch[4];
    }un;
    
    un.u32 = 0x12345678;
    
    return (0x12 == un.uch[0]);
}

//uint16_t hton16(uint16_t h16)
//{
//    return IsBigEndian() ? h16 : EndianSwap16(h16);
//}
//
//uint32_t hton32(uint32_t h32)
//{
//    return IsBigEndian() ? h32 : EndianSwap16(h32);
//}
//
//uint64_t hton64(uint64_t h64)
//{
//    return IsBigEndian() ? h64 : EndianSwap16(h64);
//}
//
//uint16_t ntoh16(uint16_t n16)
//{
//    return IsBigEndian() ? n16 : EndianSwap16(n16);
//}
//
//uint32_t ntoh32(uint32_t n32)
//{
//    return IsBigEndian() ? n32 : EndianSwap16(n32);
//}
//
//uint64_t ntoh64(uint64_t n64)
//{
//    return IsBigEndian() ? n64 : EndianSwap16(n64);
//}

void strncopyn(char *dest, int destlen, const char *src, int srclen)
{
    if (srclen < destlen)
    {
        strncpy(dest, src, srclen);
        dest[srclen] = '\0';
    }
    else
    {
        strncpy(dest, src, destlen - 1);
        dest[destlen - 1] = '\0';
    }
}

// return: <= 0  error
//        == 1, an empty string
//        > 1, an nonempty string
template<typename x>
int readString_p(const char *d, int len, x *out)
{
    out->clear();
    int i = 0;
    for (i = 0; i < len; i++, ++d)
    {
        if (*d == 0)
        {
            break;
        }
        else
        {
            out->push_back(*d);
        }
    }
    // err
    if (i >= len)
    {
        return 0;
    }
    // ok, a empty string, 1 means '\0' used one bytes
    if (i == 0 && *d == 0)
    {
        return 1;
    }
    //0 < i < len, ok, a none empty string, return bytes readed include '\0' 
    return i + 1;
}

int readString(const char *d, int len, std::string *out)
{
    return readString_p(d, len, out);
}

int readString(const char *d, int len, std::vector<char> *out)
{
    return readString_p(d, len, out);
}

static void splitString(char *str, const char *sp, std::vector<string> *strlist)
{
    char *saver;
    char *token;
    char *p = str;
    for (;;p = NULL)
    {
       token = strtok_r(p, sp, &saver);
       if (token == NULL)
       {
           break;
       }
       strlist->push_back(std::string(token));
    }
}

void splitString(const std::string &str, const std::string  &sp,std::vector<string> *strlist)
{
    return ::splitString((char *)str.c_str(), (const char *)sp.c_str(), strlist);
}

std::string timet2String(time_t time)
{
    struct tm t;
    struct tm *r = localtime_r(&time, &t);
    if (r == NULL)
        return std::string("");
    char buf[200];
    if (strftime(buf, sizeof(buf), "%F %T", &t) == 0)
    {
        return std::string("");
    }
    return std::string(buf);
}

bool Util1::readUint8(char **buf, int *len, uint8_t *v)
{
    if (*len < 1)
        return false;
    *v = uint8_t(**buf);
    (*buf)++;
    (*len)--;
    return true;
}

bool Util1::readUint16(char **buf, int *len, uint16_t *v)
{
    if ((*len) < 2)
        return false;
    *v = uint16_t(*((uint16_t*)(*buf)));
    (*buf) += 2;
    (*len) -= 2;
    return true;
}

bool Util1::readUint32(char **buf, int *len, uint32_t *v)
{
    if ((*len) < 4)
        return false;
    *v = uint32_t(*((uint32_t*)(*buf)));
    (*buf) += 4;
    (*len) -= 4;
    return true;

}

bool Util1::readUint64(char **buf, int *len, uint64_t *v)
{
    if ((*len) < 8)
        return false;
    *v = uint64_t(*((uint64_t*)(*buf)));
    (*buf) += 8;
    (*len) -= 8;
    return true;

}

bool Util1::readString(char **buf, int *len, std::string *v)
{
    uint32_t strlen = 0;
    if (!readUint32(buf, len, &strlen))
    {
        return false;
    }
    if (uint32_t(*len) < strlen)
        return false;
    *v = std::string(*buf, int(strlen));
    (*buf) += strlen;
    *len -= strlen;
    return true;
}
