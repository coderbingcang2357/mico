package micom.rest;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.core.Ordered;
import org.springframework.web.servlet.View;
import org.springframework.web.servlet.ViewResolver;
import org.springframework.web.servlet.view.InternalResourceViewResolver;
import org.springframework.web.servlet.view.json.MappingJackson2JsonView;

import java.util.Locale;

/**
 * Created by wqs on 9/7/16.
 */
public class RestViewResolver implements ViewResolver, org.springframework.core.Ordered {
    //private InternalResourceViewResolver resolver;
    @Autowired
    private MappingJackson2JsonView jksview;
    private int order;
    public void setOrder(int order) {
        this.order = order;
    }

    public int getOrder()
    {
        return order;
    }

    public RestViewResolver()
    {
        //resolver = new InternalResourceViewResolver();
        //resolver.setPrefix("/WEB-INF/views/");
        //resolver.setSuffix(".jsp");
        //resolver.setExposeContextBeansAsAttributes(true);
    }

    public View resolveViewName(String viewname, Locale l) throws Exception {
        if (viewname.startsWith("rest-"))
            return jksview;
        else
            return null;//resolver.resolveViewName(viewname, l);
    }
}
