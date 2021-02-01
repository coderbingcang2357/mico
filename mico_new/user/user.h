#ifndef USER__H_
#define USER__H_
#include <memory>
#include <string>
#include <list>
#include <stdint.h>
#include <vector>

class User
{
class UserInfo
{
public:
    uint64_t userid;
    std::string pwd;
    std::string account;
    std::string phone;
    std::string nickName;
    std::string mail;
    std::string signature;
    uint8_t headNumber;
};

public:
    bool fromByteArray(char *buf, uint32_t buflen);
    void toByteArray(std::vector<char> *d);

    User();
    User(const UserInfo &u);
    UserInfo info;
    std::list<uint64_t> clusterlist;
};

#endif
