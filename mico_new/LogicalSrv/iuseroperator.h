#ifndef IUSEROPERATOR__H
#define IUSEROPERATOR__H

#include <stdint.h>
#include <string>

class UserInfo
{
public:
    uint64_t userid;
    std::string account;
    std::string pwd;
    std::string phone;
    std::string nickName;
    std::string mail;
    std::string signature;
    uint8_t headNumber;
};

class IUserOperator
{
public:
    virtual int getUserPassword(uint64_t userid, std::string *password) = 0;
    virtual int getUserPassword(const std::string &usraccount, std::string *password) = 0;
    virtual bool isUserExist(const std::string &usraccount) = 0;
    virtual UserInfo *getUserInfo(uint64_t userid) = 0;
    virtual int getUserIDByAccount(const std::string &usraccount, uint64_t *userid) = 0;
};
#endif
