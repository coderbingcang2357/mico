package config;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.core.env.Environment;
import org.springframework.data.redis.connection.jedis.JedisConnectionFactory;
import org.springframework.data.redis.connection.jedis.JedisConnection;
import org.springframework.data.redis.core.RedisTemplate;
import org.springframework.data.redis.core.StringRedisTemplate;
import redis.clients.jedis.JedisPoolConfig;
import redis.clients.jedis.JedisShardInfo;

/**
 * Created by wqs on 7/22/16.
 @ComponentScan(basePackages = {"micom", "model"})
 */
@Configuration
public class RedisConfig {
    @Autowired
    Environment env;
    @Bean
    public JedisConnectionFactory jedisConnFactory()
    {
        JedisConnectionFactory fac = new JedisConnectionFactory();
        fac.setHostName(env.getProperty("redis.ip", "127.0.0.1"));
        fac.setPort(Integer.valueOf(env.getProperty("redis.port", "6379")).intValue());
        fac.setPassword(env.getProperty("redis.pwd", ""));
        //fac.setHostName("127.0.0.1");
        //fac.setPort(13879);
        //fac.setHostName("10.1.240.39");
        //fac.setPort(6379);
        fac.setUsePool(true);
        //fac.setPassword("3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb");
        // 必需要设置这个,否则调用fac.getconnection时会抛异常
        // fetchJedisConnector函数会用到JedisShardInfo, 如果为空就会出NullPointerException
        //JedisShardInfo jsi = new JedisShardInfo("10.1.240.39", 6379);
        //jsi.setPassword("3ETkyXt6BFFgVf8lJOaomWLPRiXs7Dxb");
        //fac.setShardInfo(jsi);
        //fac.afterPropertiesSet();
        return fac;
    }

    @Bean
    public StringRedisTemplate redisTemplate()
    {
        StringRedisTemplate st = new StringRedisTemplate(jedisConnFactory());
        return st;
    }
}
