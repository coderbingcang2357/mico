#ifndef REDIS_CONN_POOL_H
#define REDIS_CONN_POOL_H

#include <memory>
#include <errno.h>
#include <string.h>
#include "hiredis/hiredis.h"
#include "config/configreader.h"
#include "redisconnectionpool/redisconnectionpool.h"

class RedisConnectionPool;

class RedisConnPool 
{
	public:
    static std::shared_ptr<RedisConnPool> get_instance();

    RedisConnPool();
    ~RedisConnPool();
	void Set(std::string key, std::string &value);
	void Get(std::string key, std::string &value);
	void Expire(std::string key, std::uint32_t timeout);

	private:
	std::shared_ptr<RedisConnectionPool> pool;
};

#endif
