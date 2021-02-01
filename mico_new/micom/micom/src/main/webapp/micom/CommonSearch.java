package micom;

import model.CommonSearchModel;
import model.User;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.stereotype.Component;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;

import java.util.List;

import static org.springframework.web.bind.annotation.RequestMethod.GET;
import static org.springframework.web.bind.annotation.RequestMethod.POST;

/**
 * Created by wqs on 12/28/16.
 */
@Controller
public class CommonSearch {

    @Autowired
    private CommonSearchModel cs;
    @RequestMapping(value="/search", method=GET)
    public String commonSearch(@RequestParam(value="name", defaultValue = "") String name,
                               @RequestParam(value="update", defaultValue = "") String up,
                               Model model) {
        boolean isupdate = up.equals("true") ? true : false;
        List<CommonSearchModel.IndexItem> rs = cs.get(name.toLowerCase(), isupdate);
        model.addAttribute("rs", rs);
        int size = rs == null ? 0 : rs.size();
        model.addAttribute("size", size);
        return "commonsearch";
    }
}
