package micom.rest;

import model.Logreader;
import model.ServerInfoRepos;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;

/**
 * Created by wqs on 2/10/17.
 */
@Controller
public class MicoLoginfo {
    @Autowired
    private ServerInfoRepos rep;

    @RequestMapping(value="/rest/getlog", method= RequestMethod.GET)
    public String getUserList(@RequestParam("server") int server,
                              @RequestParam("size") int size,
                              Model m) {
        String s = rep.getServerIp(server);
        String logs = Logreader.readlog(s, size);
        m.addAttribute("res", "success");
        m.addAttribute("log", logs);
        return "rest-log";
    }
}

