package micom;

import model.SearchUserModel;
import model.UserInfo;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;

import java.util.List;

import static org.springframework.web.bind.annotation.RequestMethod.GET;
import static org.springframework.web.bind.annotation.RequestMethod.POST;

/**
 * Created by wqs on 7/26/16.
 */
@Controller
public class SearchUser {

    @Autowired
    private SearchUserModel suser;

    @RequestMapping(value="/searchuser", method=GET)
    public String searchUser() {
        return "searchuser";
    }

    @RequestMapping(value="/searchuser", method=POST)
    public String searchUserPost(@RequestParam(value="name", defaultValue = "") String name, Model model) {
        List<model.User> lu = suser.searchUser(name);
        model.addAttribute("susers", lu);
        return "userinfo";
    }

    @RequestMapping(value="/userinfo/{id}", method=GET)
    public String getUserInfo(@PathVariable("id") long id, Model model) {
        List<model.User> lu = suser.searchUser(id);
        model.addAttribute("susers", lu);
        return "userinfo";
    }
}
