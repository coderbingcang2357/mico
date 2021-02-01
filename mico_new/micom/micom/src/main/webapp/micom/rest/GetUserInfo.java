package micom.rest;

import com.fasterxml.jackson.core.JsonFactory;
import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.JsonParser;
import com.fasterxml.jackson.core.JsonToken;
import model.*;
import org.apache.catalina.Cluster;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;

import java.io.IOException;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Created by wqs on 9/7/16.
 */
@Controller
public class GetUserInfo {

    @Autowired
    private UsersModel usersModel;
    @Autowired
    private DevicesModel devmodel;
    @Autowired
    private ClustersModel clumodel;
    @Autowired
    private CommonSearchModel cs;

    @RequestMapping(value="/rest/getui", method= RequestMethod.GET)
    public String getUserList(@RequestParam("page") int page,
                              @RequestParam("pagesize") int pagesize,
                              Model m){
        if (page < 0)
            page = 0;
        if (pagesize < 0 || pagesize > 200)
        {
            pagesize = 100;
        }
        util.Pair<List<UserInfo>, Integer> users = usersModel.getUsers(page, pagesize);
        m.addAttribute("count", users.second);
        m.addAttribute("users", users.first);
        return "rest-getui";
    }

    @RequestMapping(value="/rest/getdev", method= RequestMethod.GET)
    public String getDevList(@RequestParam("page") int page,
                              @RequestParam("pagesize") int pagesize,
                              Model m){
        if (page < 0)
            page = 0;
        if (pagesize < 0 || pagesize > 200)
        {
            pagesize = 100;
        }
        util.Pair<List<DeviceInfo>, Integer> users = devmodel.getDevs(page, pagesize);
        m.addAttribute("count", users.second);
        m.addAttribute("devs", DeviceInfo_IdStr.fromList(users.first));
        return "rest-getdev";
    }

    private static class DeviceInfo_IdStr{
        private long id;
        private String id_str;
        private String name;
        private long firmClusterID;
        private String macAddr;

        private String clusterID;
        private String loginKey;

        public String getClusterID() {
            return clusterID;
        }

        public void setClusterID(String clusterID) {
            this.clusterID = clusterID;
        }

        public static List<DeviceInfo_IdStr> fromList(List<DeviceInfo> l) {
            List<DeviceInfo_IdStr> li = new ArrayList<DeviceInfo_IdStr>();
            for (DeviceInfo di : l) {
                DeviceInfo_IdStr dis = new DeviceInfo_IdStr();
                dis.id = di.getId();
                dis.id_str = String.valueOf(dis.id);
                dis.name = di.getName();
                dis.firmClusterID = di.getFirmClusterID();
                dis.macAddr = di.getMacAddr();
                dis.clusterID = Long.toUnsignedString(di.getClusterID());
                li.add(dis);
            }
            return li;
        }

        public long getId() {
            return id;
        }

        public void setId(long id) {
            this.id = id;
        }

        public String getId_str() {
            return id_str;
        }

        public void setId_str(String id_str) {
            this.id_str = id_str;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public long getFirmClusterID() {
            return firmClusterID;
        }

        public void setFirmClusterID(long firmClusterID) {
            this.firmClusterID = firmClusterID;
        }

        public String getMacAddr() {
            return macAddr;
        }

        public void setMacAddr(String macAddr) {
            this.macAddr = macAddr;
        }

        public String getLoginKey() {
            return loginKey;
        }

        public void setLoginKey(String loginKey) {
            this.loginKey = loginKey;
        }

    }

    @RequestMapping(value="/rest/getcluster", method= RequestMethod.GET)
    public String getClusterList(@RequestParam("page") int page,
                             @RequestParam("pagesize") int pagesize,
                             Model m){
        if (page < 0)
            page = 0;
        if (pagesize < 0 || pagesize > 200)
        {
            pagesize = 100;
        }
        util.Pair<List<ClusterInfo>, Integer> users = clumodel.getClusters(page, pagesize);
        m.addAttribute("count", users.second);
        m.addAttribute("clus", ClusterInfo_IdStr.fromList(users.first));
        return "rest-getclu";
    }

    private static class ClusterInfo_IdStr {
        public long getId() {
            return id;
        }

        public void setId(long id) {
            this.id = id;
        }

        public String getId_str() {
            return id_str;
        }

        public void setId_str(String id_str) {
            this.id_str = id_str;
        }

        public String getAccount() {
            return account;
        }

        public void setAccount(String account) {
            this.account = account;
        }

        public String getNotename() {
            return notename;
        }

        public void setNotename(String notename) {
            this.notename = notename;
        }

        public String getDescrib() {
            return describ;
        }

        public void setDescrib(String describ) {
            this.describ = describ;
        }

        public long getCreatorID() {
            return creatorID;
        }

        public void setCreatorID(long creatorID) {
            this.creatorID = creatorID;
        }

        public long getSysAdminID() {
            return sysAdminID;
        }

        public void setSysAdminID(long sysAdminID) {
            this.sysAdminID = sysAdminID;
        }

        public Date getCreateDate() {
            return createDate;
        }

        public void setCreateDate(Date createDate) {
            this.createDate = createDate;
        }

        private long id;
        private String id_str;
        private String account;
        private String notename;
        private String describ;
        private long creatorID;
        private long sysAdminID;
        private Date createDate;

        public static List<ClusterInfo_IdStr> fromList(List<ClusterInfo> l) {
            List<ClusterInfo_IdStr> lcii = new ArrayList<ClusterInfo_IdStr>();
            for (ClusterInfo ci : l) {
                ClusterInfo_IdStr cii = new ClusterInfo_IdStr();
                cii.id = ci.getId();
                cii.id_str = String.valueOf(cii.id);
                cii.account = ci.getAccount();
                cii.notename = ci.getNotename();
                cii.describ = ci.getDescrib();
                cii.creatorID = ci.getCreatorID();
                cii.sysAdminID = ci.getSysAdminID();
                cii.createDate = ci.getCreateDate();
                lcii.add(cii);
            }
            return lcii;
        }
    }
    @RequestMapping(value="/rest/modifyuserpassword", method = RequestMethod.POST)
    public String modifyUserPassowrd(@RequestParam("id") long userid,
                                 @RequestParam("newpwd") String newpwd,
                                 Model m) {
        if (usersModel.modifyUserPassword(userid, newpwd)) {
            m.addAttribute("res", "success");
        } else {
            m.addAttribute("res", "failure");
        }
        return "rest-modifyuserpassword";
    }

    @RequestMapping(value="/rest/modifyusermail", method = RequestMethod.POST)
    public String modifyUserMail(@RequestParam("id") long userid,
                                 @RequestParam("newmail") String newmail,
                                 Model m) {
        if (usersModel.modifyUserMail(userid, newmail)) {
            m.addAttribute("res", "success");
        } else {
            m.addAttribute("res", "failure");
        }
        return "rest-modifyusermail";
    }

    @RequestMapping(value="/rest/createcluster", method= RequestMethod.POST)
    public String createCluster(@RequestParam("userid") long userid,
                                @RequestParam("name") String name,
                                 @RequestParam("desc") String desc,
                                 Model m){
        Object [] res = usersModel.createCluster(userid, name, desc);
        boolean r = ((Boolean)res[0]).booleanValue();
        long id = ((Long)res[1]).longValue();
        if (r) {
            m.addAttribute("res", "success");
        } else {
            m.addAttribute("res", "failure");
        }
        m.addAttribute("clusterid", id);
        return "rest-createcluster";
    }

    @RequestMapping(value="/rest/geuserinfobyid", method= RequestMethod.GET)
    public String getUserList(@RequestParam("userid") long userid,
                              Model m){
        return "rest-getuserinfobyid";
    }

    @RequestMapping(value="/rest/deviceAuthorize", method= RequestMethod.POST)
    public String deviceAuthorize(@RequestParam("userid") String userid,
                                  @RequestParam("devid") long devid,
                                  @RequestParam("adminid") long adminid,
                              Model m){
        String res;
        try
        {
            ArrayList<Long> userids = new ArrayList<Long>();
            JsonFactory jsonFactory = new JsonFactory(); // or, for data binding, org.codehaus.jackson.mapper.MappingJsonFactory
            JsonParser jp = jsonFactory.createParser(userid); // or URL, Stream, Reader, String, byte[]
            JsonToken t = jp.nextToken();
            if (t.equals(JsonToken.START_ARRAY)) {
                t = jp.nextToken();
                while (!jp.isClosed() && !t.equals(JsonToken.END_ARRAY)) {
                    if (t.equals(JsonToken.VALUE_STRING)) {
                        String id = jp.getText().trim();
                        userids.add(Long.valueOf(id));
                    }

                    t = jp.nextToken();
                }
            } else {
                // error
                throw new Exception("json parseing error.");
            }
            // userid is a json object
            //
            res = clumodel.deviceAuthorize(adminid, devid, userids);
        }
        catch (IOException e1) {
            res = e1.toString();
        }
        catch (Exception e) {
            res = e.toString();
        }

        m.addAttribute("res", res);
        return "rest-deviceAuthorize";
    }

    @RequestMapping(value="/rest/commonsearch", method= RequestMethod.POST)
    public String commonSearch(@RequestParam("text") String text,
                                  Model m) {
        List<CommonSearchModel.IndexItem> rs = cs.get(text.toLowerCase(), false);
        m.addAttribute("searchres", rs);
        m.addAttribute("res", "success");
        return "rest-commonsearch";
    }

    @RequestMapping(value="/rest/addusertocluster", method= RequestMethod.POST)
    public String addUserToCluster(@RequestParam("users") String users,
                                   @RequestParam("clusterid") long clusterid,
                               Model m) {
        //m.addAttribute("searchres", rs);
        String res = "success";
        try {
            //
            ArrayList<Long> userids = new ArrayList<Long>();
            // parse users, it's a array json string:
            // [{"id":34,"account":"o.text","isSelected":true}]
            JsonFactory jsonFactory = new JsonFactory(); // or, for data binding, org.codehaus.jackson.mapper.MappingJsonFactory
            JsonParser jp = jsonFactory.createParser(users); // or URL, Stream, Reader, String, byte[]
            JsonToken t = jp.nextToken();
            if (t.equals(JsonToken.START_ARRAY)) {
                t = jp.nextToken();
                while (!jp.isClosed() && !t.equals(JsonToken.END_ARRAY)) {
                    if (t.equals(JsonToken.START_OBJECT)) {
                        t = jp.nextToken();
                        // read a object
                        while (!jp.isClosed() && !t.equals(JsonToken.END_OBJECT)) {
                            if (t.equals(JsonToken.FIELD_NAME) && "id".equals(jp.getCurrentName())) {
                                t = jp.nextToken();
                                String id = jp.getText().trim();
                                userids.add(Long.valueOf(id));
                                break;
                            }
                            t = jp.nextToken();
                        }
                    }
                    else {
                        t = jp.nextToken();
                    }
                }
            }
            res = clumodel.addUserToCluster(clusterid, userids);
        } catch (Exception e) {
            res = e.toString();
        }
        m.addAttribute("res", res);
        return "rest-commonsearch";
    }
}
