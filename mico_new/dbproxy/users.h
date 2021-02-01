#ifndef USERS__H_
#define USERS__H_
#include <memory>
#include <map>
#include "thread/mutexwrap.h"

class User;
class Users
{
private:
    Users();
    static Users users;
public:
    Users(const Users &) = delete;
    Users(const Users &&) = delete;
    const Users operator = (const Users &) = delete;

    static Users *instance();


    std::shared_ptr<User> getByID(uint64_t id);
    std::shared_ptr<User> getByAccount(const std::string &account);
    void insert(const std::shared_ptr<User> &u);

private:
    MutexWrap mutexIduser;
    MutexWrap mutexAccountuser;

    std::map<uint64_t, std::shared_ptr<User> > m_iduser;
    std::map<std::string, std::shared_ptr<User> > m_accountuser;
};
#endif
