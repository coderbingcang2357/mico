package micom;

import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

/**
 * Created by wqs on 7/26/16.
 */
@Controller
public class ServerList {

    @RequestMapping(value="/serverlist", method = RequestMethod.GET)
    public String serverlist() {

        return "serverlist";
    }
}
