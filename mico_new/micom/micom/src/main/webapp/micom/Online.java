package micom;

import static org.springframework.web.bind.annotation.RequestMethod.*;

import model.OnlineModel;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.core.userdetails.UserDetails;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.beans.factory.annotation.Autowired;

@Controller
@RequestMapping({"/", "/online"})
public class Online
{
    private final Logger log = LogManager.getLogger("micom");
    //@Autowired
    //ServletContext context;
    //String lg = context.getRealPath("/") + File.separator + "WEB-INF/"+"log4j2.xml";
    //Configurator.initialize(null, lg);
    @Autowired
    private OnlineModel om;

    public Online()
    {
    }

    @RequestMapping(method=GET)
    public String online(Model model)
    {
        Object principal = SecurityContextHolder.getContext().getAuthentication().getPrincipal();

        String username;
        if (principal instanceof UserDetails) {
            username = ((UserDetails)principal).getUsername();
        } else {
            username = principal.toString();
        }
        log.debug("user " + username + "access onlie");
        model.addAttribute("username", username);

        model.addAttribute("onlinelist", om.getClient());
        return "online";
    }
}

