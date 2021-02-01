#ifndef passworddao__H__
#define passworddao__H__
#include <stdint.h>
#include <string>
class PasswordDao
{
public:
    int getPasswordAndID(const std::string &account,
                        uint64_t *id,
                        std::string *pwd);

    int modifyPassword(const std::string &account, uint64_t uid, const std::string &newpwd);

private:
    int getPasswordAndIDFromRedis(const std::string &account,
                        uint64_t *id,
                        std::string *pwd);

    int getPasswordAndIDFromMysql(const std::string &account,
                        uint64_t *id,
                        std::string *pwd);

    int savePasswordAndIdToRedis(const std::string &account,
                        uint64_t id,
                        const std::string &pwd);
};
#endif

