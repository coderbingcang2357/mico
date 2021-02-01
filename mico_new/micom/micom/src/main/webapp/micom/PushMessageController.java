package micom;

import model.PushMessageService;
import model.PushMessge;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;

import java.util.List;

/**
 * Created by wqs on 8/25/17.
 */
@Controller
public class PushMessageController {
    @Autowired
    PushMessageService pms;
    @RequestMapping(value={"/pushmessage"}, method = RequestMethod.GET)
    public String pushmessage() {
        return "pushmessage";
    }
    @RequestMapping(value={"/pushmessage-getall"}, method = RequestMethod.POST)
    public String getAllPushmessage(Model m) {
        List<PushMessge> messages = pms.getPushMessage();
        m.addAttribute("res", "success");
        m.addAttribute("messages", messages);
        return "rest-pushmessage-getall";
    }

    @RequestMapping(value={"/pushmessage-del"}, method = RequestMethod.POST)
    public String delPushmessage(@RequestParam("id") long id, Model mode) {

        if (pms.deletePushMessage(id))
        {
            mode.addAttribute("res", "success");
        }
        else
        {
            mode.addAttribute("res", "failure");
        }
        return "rest-pushmessage-del";
    }
    @RequestMapping(value={"/pushmessage-insert"}, method = RequestMethod.POST)
    public String insertPushmessage(@RequestParam("message") String message, Model mode) {
        long retid = pms.insertPushMessage(message.trim());
        if (retid == 0)
        {
            mode.addAttribute("res", "failure");
        }
        else
        {
            mode.addAttribute("res", "success");
            mode.addAttribute("id", retid);
        }
        return "rest-pushmessage-insert";
    }

    @RequestMapping(value={"/pushmessage-send"}, method = RequestMethod.POST)
    public String insertPushmessage(@RequestParam("id") long msgid, Model mode) {
        boolean isok = pms.sendPushMesssage(msgid);
        if (!isok)
        {
            mode.addAttribute("res", "failure");
        }
        else
        {
            mode.addAttribute("res", "success");
        }
        return "rest-pushmessage-send";
    }
}
