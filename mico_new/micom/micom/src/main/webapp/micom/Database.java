package micom;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

/**
 * Created by wqs on 12/23/16.
 */
@Controller
public class Database {
    @RequestMapping(value="/db", method = RequestMethod.GET)
    public String dbview()
    {
        return "db";
    }
}
