package micom;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

/**
 * Created by wqs on 7/26/16.
 */
// 根路径在 online 中处理了, 这个类没有用...
@Controller
@RequestMapping(value = {"/home"})
public class Home {
    @RequestMapping(method = RequestMethod.GET)
    public String home() {
        return "home";
    }
}
