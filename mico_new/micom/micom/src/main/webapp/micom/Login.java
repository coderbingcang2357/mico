package micom;

import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

import java.net.URL;
import java.util.Enumeration;

/**
 * Created by wqs on 12/19/16.
 */
@Controller
public class Login {
    @RequestMapping(value="/login", method = RequestMethod.GET)
    public String login(Model model)
    {
        //String path = new String();
        //try {
        //    Enumeration<URL> urls = Thread.currentThread().getContextClassLoader().getResources("org");
        //    while (urls.hasMoreElements()) {
        //        URL url = urls.nextElement();
        //        path += url.getPath();
        //    }
        //}
        //catch (Exception e) {
        //}
        //model.addAttribute("classpath", path);
        return "login";
    }
}
