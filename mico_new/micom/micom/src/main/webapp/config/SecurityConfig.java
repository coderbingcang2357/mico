package config;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.ImportResource;
import org.springframework.security.config.annotation.authentication.builders.AuthenticationManagerBuilder;
import org.springframework.security.config.annotation.web.builders.HttpSecurity;
import org.springframework.security.config.annotation.web.configuration.EnableWebSecurity;
import org.springframework.security.config.annotation.web.configuration.WebSecurityConfigurerAdapter;
import org.springframework.security.core.userdetails.User;
import org.springframework.security.core.userdetails.UserDetailsService;
import org.springframework.security.provisioning.InMemoryUserDetailsManager;
import org.springframework.security.web.util.matcher.AntPathRequestMatcher;


/**
 * Created by wqs on 12/16/16.
 @EnableWebSecurity
 */
@Configuration
public class SecurityConfig { //extends WebSecurityConfigurerAdapter{
    //@Override
    //protected void configure(AuthenticationManagerBuilder auth) throws Exception
    //{
    //    auth.inMemoryAuthentication()
    //            .withUser("liushenghong@ct.com").password("x,.ewIO(*&").roles("USER", "ADMIN").and()
    //            .withUser("wangqiaosheng@ct.com").password("ajkdeuOOP%^))").roles("USER", "ADMIN").and()
    //            .withUser("ot@ct.com").password("LIuiU88*(").roles("USER");
    //}
    //@Override
    //protected void configure(HttpSecurity http) throws Exception
    //{
    //    http.csrf().disable();
    //    http.formLogin().loginPage("/login").failureForwardUrl("/login").and()
    //            .logout().logoutRequestMatcher(new AntPathRequestMatcher("/logout", "GET")) // 退出用GET请求
    //                    .logoutSuccessUrl("/login").and()
    //            .authorizeRequests().antMatchers("/login").permitAll()
    //            //.antMatchers("/serverlist").hasRole("ADMIN")
    //            .antMatchers("/resources/**").permitAll()
    //            .anyRequest().hasRole("USER")
    //            .and().exceptionHandling().accessDeniedPage("/noauth.html"); // 没有权限的页面跳转到这里
    //}
    //@Bean
    //public UserDetailsService userDetailsService() {
    //    InMemoryUserDetailsManager manager = new InMemoryUserDetailsManager();
    //    manager.createUser(User.withUsername("user").password("password").roles("USER").build());
    //    return manager;
    //}
}
