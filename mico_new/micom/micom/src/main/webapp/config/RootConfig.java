package config;

import org.springframework.context.annotation.*;
import org.springframework.context.annotation.ComponentScan.Filter;
import org.springframework.context.support.PropertySourcesPlaceholderConfigurer;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.web.servlet.config.annotation.EnableWebMvc;

import javax.sql.DataSource;


@Configuration
@ComponentScan(basePackages={"micom", "model", "micoserver"},
    excludeFilters={@Filter(type=FilterType.ANNOTATION, value=EnableWebMvc.class)})
@ImportResource({"WEB-INF/config.xml", "WEB-INF/security.xml"})
@PropertySource("file:/data/web.properties")
public class RootConfig
{
    @Bean
    public JdbcTemplate getJdbcTemplate(DataSource ds) {
        return new JdbcTemplate(ds);
    }

    @Bean
    public PropertySourcesPlaceholderConfigurer placeholderConfigurer() {
        return new PropertySourcesPlaceholderConfigurer();
    }
}
