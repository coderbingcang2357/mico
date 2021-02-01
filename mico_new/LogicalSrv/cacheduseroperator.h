#ifndef CACHEDUSEROPERATOR__H
#define CACHEDUSEROPERATOR__H
#include "iuseroperator.h"


class CachedUserOperator : public IUserOperator
{
public:
    CachedUserOperator();

    int getUserPassword(uint64_t userid, std::string *password);
    int getUserPassword(const std::string &usraccount, std::string *password);
    bool isUserExist(const std::string &usraccount);
    UserInfo *getUserInfo(uint64_t userid);
    UserInfo *getUserInfo(const std::string &useraccount);
    int getUserIDByAccount(const std::string &usraccount, uint64_t *userid);
};

#endif
