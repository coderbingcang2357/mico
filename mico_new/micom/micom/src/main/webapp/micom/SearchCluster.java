package micom;

import model.ClusterInfo;
import model.SearchClusterModel;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;

import javax.websocket.server.PathParam;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
@Controller
public class SearchCluster {

    @Autowired
    private SearchClusterModel searchclu;

    @RequestMapping(value = "/searchcluster", method = RequestMethod.POST)
    public String searchClusterByName(@RequestParam("name") String name, Model mode) {
        mode.addAttribute("clusters", searchclu.searchClusterByName(name));
        return "clusterinfo";
    }

    @RequestMapping(value = "/clusterinfo/{id}", method = RequestMethod.GET)
    public String searchClusterByID(@PathVariable("id") Long id, Model mode) {
        List<ClusterInfo> cl=null;
        if (id != null)
            cl = searchclu.searchClusterByID(id);
        mode.addAttribute("clusters", cl);
        return "clusterinfo";
    }
}
