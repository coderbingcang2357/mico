#include "util/util.h"
#include "user.h"

User::User()
{
}

User::User(const UserInfo &u) : info(u)
{
}

// uint64_t userid;
// std::string pwd
// std::string account;
// std::string phone;
// std::string nickName;
// std::string mail;
// std::string signature;
// uint8_t headNumber;
bool User::fromByteArray(char *buf, uint32_t buflen)
{
    // id
    if (buflen < sizeof(uint64_t))
        return false;
    info.userid = ReadUint64(buf);
    buf += sizeof(uint64_t);
    buflen -= sizeof(uint64_t);

    // pwd
    int readlen = ::readString(buf, buflen, &info.pwd);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // acc
    readlen = ::readString(buf, buflen, &info.account);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // phone
    readlen = ::readString(buf, buflen, &info.phone);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // nick
    readlen = ::readString(buf, buflen, &info.nickName);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // sig
    readlen = ::readString(buf, buflen, &info.signature);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // mail
    readlen = ::readString(buf, buflen, &info.mail);
    if (readlen <= 0)
        return false;
    buf += readlen;
    buflen -= readlen;

    // head
    if (buflen < sizeof(uint8_t))
        return false;

    info.headNumber =  ReadUint8(buf);

    return true;
}

// uint64_t userid;
// std::string pwd
// std::string account;
// std::string phone;
// std::string nickName;
// std::string mail;
// std::string signature;
// uint8_t headNumber;
void User::toByteArray(std::vector<char> *d)
{
    WriteUint64(d, info.userid);
    WriteString(d, info.pwd);
    WriteString(d, info.account);
    WriteString(d, info.phone);
    WriteString(d, info.nickName);
    WriteString(d, info.signature);
    WriteString(d, info.mail);
    WriteUint8(d, info.headNumber);
}

