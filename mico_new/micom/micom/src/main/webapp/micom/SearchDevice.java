package micom;

import model.DeviceInfo;
import model.SearchDeviceModel;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;

import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
@Controller
public class SearchDevice {
    @Autowired
    SearchDeviceModel searchDevice;

    @RequestMapping(value = "/searchdev", method = RequestMethod.POST)
    public String searchDevice(@RequestParam("name") String devname, Model mode) {
        List<DeviceInfo> devs = searchDevice.searchDeviceByName(devname);
        mode.addAttribute("devs", devs);
        return "devinfo";
    }

    @RequestMapping(value = "/devinfo/{id}", method = RequestMethod.GET)
    public String getDeviceInfo(@PathVariable("id") Long id, Model mode) {
        List<DeviceInfo> devs = searchDevice.searchDeviceByID(id);
        mode.addAttribute("devs", devs);
        return "devinfo";
    }
}
