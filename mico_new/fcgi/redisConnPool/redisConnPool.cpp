#include "redisConnPool.h"
#include "../../util/logwriter.h"

RedisConnPool::RedisConnPool(){
	Configure *cfg = Configure::getConfig();
	pool = std::make_shared<RedisConnectionPool>(cfg->redis_address,cfg->redis_port,cfg->redis_pwd);
}

RedisConnPool::~RedisConnPool(){
	
}

std::shared_ptr<RedisConnPool> RedisConnPool::get_instance(){
	static RedisConnPool pool;
	return std::make_shared<RedisConnPool>(pool);
}

void RedisConnPool::Set(std::string key, std::string &value)
{
	// 添加到redis中
	std::shared_ptr<redisContext> ptrc = pool->get();
	redisContext *c = ptrc.get();
	if (c)
	{    
		// write to redis
		redisReply *reply;
		reply = (redisReply *)redisCommand(c,
				"set %s %s",
				key.c_str(), value.c_str());

		if (reply == 0 || REDIS_REPLY_ERROR == reply->type )
		{    
			// error
			if (c->err == REDIS_ERR_IO)
			{    
				//   
				printf("io error:%s\n", strerror(errno));
				if (errno == EPIPE)
				{    
					// connection closed
					printf("redis conection closed.\n");
				}    
			}    
			else 
			{    
				printf("redis command error: %s\n", c->errstr);
			}    
		}    
		else // ok
		{    
			//.  
		}    
		if (reply != 0)
			freeReplyObject(reply);
	}
}
void RedisConnPool::Expire(std::string key,uint32_t timeout){

	// 添加到redis中
	std::shared_ptr<redisContext> ptrc = pool->get();
	redisContext *c = ptrc.get();
	if (c)
	{
		// write to redis
		redisReply *reply;
		reply = (redisReply *)redisCommand(c,
				"expire %s %lu",
				key.c_str(), timeout);

		if (reply == 0 || REDIS_REPLY_ERROR == reply->type )
		{
			// error
			if (c->err == REDIS_ERR_IO)
			{
				//
				printf("io error:%s\n", strerror(errno));
				if (errno == EPIPE)
				{
					// connection closed
					printf("redis conection closed.\n");
				}
			}
			else
			{
				printf("redis command error: %s\n", c->errstr);
			}
		}
		else // ok
		{
			//.
		}
		if (reply != 0)
			freeReplyObject(reply);
	}
}

void RedisConnPool::Get(std::string key, std::string &value)
{
	// 添加到redis中
	std::shared_ptr<redisContext> ptrc = pool->get();
	redisContext *c = ptrc.get();
	if (c)
	{
		// write to redis
		redisReply *reply;
		reply = (redisReply *)redisCommand(c,
				"get %s",
				key.c_str());

		if (reply == 0 || REDIS_REPLY_ERROR == reply->type )
		{
			// error
			if (c->err == REDIS_ERR_IO)
			{
				//
				printf("io error:%s\n", strerror(errno));
				if (errno == EPIPE)
				{
					// connection closed
					printf("redis conection closed.\n");
				}
			}
			else
			{
				printf("redis command error: %s\n", c->errstr);
			}
		}
		else // ok
		{
			//.
			::writelog(InfoType,"11111122222:%d",reply->type);
			if (reply->type == REDIS_REPLY_STRING)
				value = std::string(reply->str, reply->len);
		}
		if (reply != 0)
			freeReplyObject(reply);
	}
}
