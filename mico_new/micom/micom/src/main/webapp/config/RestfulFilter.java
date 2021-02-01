package config;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.security.web.access.intercept.FilterSecurityInterceptor;
import org.springframework.security.web.authentication.logout.LogoutFilter;
import org.springframework.web.filter.GenericFilterBean;

import javax.servlet.*;
import javax.servlet.http.HttpServletRequest;
import java.io.IOException;
import java.io.PrintWriter;

/**
 * Created by wqs on 2/5/17.
 */
public class RestfulFilter implements  Filter {
    @Override
    public void init(FilterConfig fc) throws ServletException {
    }

    private final Logger log = LogManager.getLogger("micom");
    @Override
    public void doFilter(ServletRequest req, ServletResponse resp, FilterChain fc)
        throws IOException, ServletException
    {
        Object principal = SecurityContextHolder.getContext().getAuthentication().getPrincipal();

        String username;
        if (principal instanceof UserDetails) {
            // 执行到这里说明有用户登录了
            username = ((UserDetails)principal).getUsername();
        } else {
            // 执行到这里说明没有用户登录, 这里取到的是字符串: "annonyuser"
            username = principal.toString();
            HttpServletRequest httpreq = (HttpServletRequest)req;
            String uri = httpreq.getRequestURI();
            if (uri.startsWith("/rest/"))
            {
                resp.setCharacterEncoding("UTF-8");
                resp.setContentType("application/json; charset=utf-8");
                PrintWriter  w = resp.getWriter();
                w.write("{\"res\":\"failure\"}");
                w.flush();w.close();
                return;
            }
        }
        //log.debug("rest filter: " + username);
        //log.debug("rest filter: " + httpreq.getRequestURI());
        // boolean isauth = httpreq.isUserInRole("USER") || httpreq.isUserInRole("ADMIN");

        fc.doFilter(req, resp);
    }

    @Override
    public void destroy() {
    }
}
