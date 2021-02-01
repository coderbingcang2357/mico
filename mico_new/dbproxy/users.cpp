#include "thread/mutexguard.h"
#include "user/user.h"
#include "users.h"

Users Users::users;
Users *Users::instance()
{
    return &Users::users;
}

Users::Users()
{
}

std::shared_ptr<User> Users::getByID(uint64_t id)
{
    MutexGuard g(&mutexIduser);
    auto it = m_iduser.find(id);
    if ( it != m_iduser.end())
    {
        return it->second;
    }
    else
    {
        return std::shared_ptr<User>();
    }
}

std::shared_ptr<User> Users::getByAccount(const std::string &account)
{
    MutexGuard g(&mutexAccountuser);
    auto it = m_accountuser.find(account);
    if (it != m_accountuser.end())
    {
        return it->second;
    }
    else
    {
        return std::shared_ptr<User>();
    }
}

void Users::insert(const std::shared_ptr<User> &u)
{
    {
    MutexGuard g(&mutexIduser);
    m_iduser[u->info.userid] = u;
    }
    {
    MutexGuard g2(&mutexAccountuser);
    m_accountuser[u->info.account] = u;
    }
}

