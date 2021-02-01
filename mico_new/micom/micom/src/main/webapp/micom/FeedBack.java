package micom;

import model.FeedBackModel;
import model.FeedbackItem;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
@Controller
public class FeedBack {

    private final int pagesize=20;
    @Autowired
    private FeedBackModel fbm;

    @RequestMapping(value = "/feedback", method = RequestMethod.GET)
    public String getFeedBackPage0(Model mode){
        int page = 0;
        util.Pair<Integer, List<FeedbackItem>> lf = fbm.getFeedBack(page, pagesize);
        int pages = (lf.first.intValue() + pagesize - 1)/ pagesize;
        mode.addAttribute("feed", lf.second);
        mode.addAttribute("page", page);
        mode.addAttribute("pages", pages);
        return "feedback";
    }

    @RequestMapping(value = "/feedback/{page}", method = RequestMethod.GET)
    public String getFeedBack(@PathVariable("page") int page,  Model mode){
        util.Pair<Integer, List<FeedbackItem>> lf = fbm.getFeedBack(page, pagesize);
        int pages = (lf.first.intValue() + pagesize - 1) / pagesize;
        mode.addAttribute("feed", lf.second);
        mode.addAttribute("page", page);
        mode.addAttribute("pages", pages);
        return "feedback";
    }
}
