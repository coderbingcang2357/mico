#include "./protocolversion.h"

// 协议中的版本字段, 高4位表示客户端类型, 其它的12位表示协议的版本
uint16_t getProtoVersion(uint16_t v)
{
    return v & 0xfff;
}

uint16_t getClientType(uint16_t v)
{
    return v >> 12;
}

std::string getClientTypeString(uint16_t clienttype)
{
    static std::string windows("windows");
    static std::string android("android");
    static std::string ios("ios");
    static std::string unknown("unknown");

    switch (clienttype)
    {
    case 0:
        return windows;

    case 1:
        return android;

    case 2:
        return ios;

    default:
        return unknown;
    }
}
