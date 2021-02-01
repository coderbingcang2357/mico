package micom;

import model.ServerInfo;
import model.ServerInfoRepos;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

import java.util.ArrayList;

/**
 * Created by wqs on 2/8/17.
 */
@Controller
public class MicoLog {
    @Autowired
    private ServerInfoRepos rep;
    @RequestMapping(value="/log", method = RequestMethod.GET)
    public String log(Model m) {
        ArrayList<ServerInfo> asi = rep.getServers();
        ArrayList<ServerInfo> newasi = new ArrayList<>();
        for (ServerInfo s : asi) {
            if (s.getPortlogical() != 0) {
                newasi.add(s);
            }
        }
        m.addAttribute("servers", newasi);
        return "log";
    }
}
