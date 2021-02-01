package config;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.data.redis.connection.jedis.JedisConnectionFactory;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.web.servlet.config.annotation.EnableWebMvc;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.web.servlet.ViewResolver;
import org.springframework.web.servlet.config.annotation.DefaultServletHandlerConfigurer;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurerAdapter;
import org.springframework.web.servlet.view.InternalResourceViewResolver;
import org.springframework.web.servlet.view.json.MappingJackson2JsonView;
import redis.clients.jedis.JedisShardInfo;

// @Import(RedisConfig.class)

@Configuration
@EnableWebMvc
@ComponentScan(basePackages = {"micom"})
public class WebConfig extends WebMvcConfigurerAdapter
{
    @Bean
    public ViewResolver viewResolver()
    {
        InternalResourceViewResolver resolver = new InternalResourceViewResolver();
        resolver.setPrefix("/WEB-INF/views/");
        resolver.setSuffix(".jsp");
        resolver.setExposeContextBeansAsAttributes(true);
        resolver.setOrder(2);
        return resolver;
    }

    @Bean
    public ViewResolver restViewResolver() {
        micom.rest.RestViewResolver resolver = new micom.rest.RestViewResolver();
        resolver.setOrder(1);
        return resolver;
    }

    @Bean
    public MappingJackson2JsonView jksView() {
        return new MappingJackson2JsonView();
    }

    @Override
    public void configureDefaultServletHandling(DefaultServletHandlerConfigurer  configurer)
    {
        configurer.enable();
    }

}
