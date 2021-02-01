#ifndef USERDAO__H__
#define USERDAO__H__
#include <stdint.h>
#include <string>
#include "passworddao.h"

class UserDao
{
public:
    UserDao();
    int getPasswordAndID(const std::string &account,
                        uint64_t *id,
                        std::string *pwd);
    int modifyPassword(const std::string &account, uint64_t id, const std::string &newpwd);

    // 最多可以创建多少个群
    int maxDeviceClusterCount();
    // 最多可以加入多少个群(包含自个创建的)
    int maxJoinDeviceClusterCount();

    // 取用户加入了多少个群(包含自己创建的)
    int getUserDeviceClusterCount(uint64_t userid, int *allclus);
    // 用户群是否达到限制
    bool isClusterLimitValid(uint64_t userid);

private:
    PasswordDao password;
    int m_maxjoinclucount;

    friend class TestUserDao;
};
#endif

