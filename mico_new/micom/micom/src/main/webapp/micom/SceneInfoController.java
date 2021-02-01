package micom;

import model.SceneModel;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

/**
 * Created by wqs on 9/8/16.
 */
@Controller
public class SceneInfoController {
    @Autowired
    private SceneModel scenemodel;

    @RequestMapping(value="/sceneinfo/{sceneid}", method = RequestMethod.GET)
    public String sceneinfo(@PathVariable("sceneid") long sceneid, Model m) {
        m.addAttribute("scenes", scenemodel.findSceneByID(sceneid));
        return "sceneinfo";
    }
}
