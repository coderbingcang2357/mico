#ifndef CACHEDDBUSEROPERATOR__H
#define CACHEDDBUSEROPERATOR__H
#include <stdint.h>
#include <string>
class DbproxyClient;
class User;

class CachedDbUserOperator
{
public:
    CachedDbUserOperator(DbproxyClient *client);
    int getUserInfo(uint64_t userid, User *u, uint16_t *answercode);
    int getUserInfo(const std::string &account, User *u, uint16_t *answercode);
    int modifyPassword(const std::string &account, const std::string &newpassword, uint16_t *answercode);

private:
    DbproxyClient *m_client;
};

#endif
