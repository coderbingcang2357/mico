#include "userofflineCommand.h"
#include "rediscache.h"

void UserOfflineCommand::execute(void *p)
{
    (void)p;
    RedisCache::instance()->removeObject(m_userid);
}
