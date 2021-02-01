package micom;

import model.PushMessge;
import model.RelayChannel;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;

import java.sql.Timestamp;
import java.util.*;

/**
 * Created by wqs on 10/31/18.
 */
@Controller
public class RelayChannelController {
    @Autowired
    private JdbcOperations jdbcoperations;


    @RequestMapping(value = {"/channels"}, method = RequestMethod.GET)
    public String getAllChannel(Model m) {
        List<RelayChannel> ch = this.getRelayChannel();
        m.addAttribute("channels", ch);
        return "relaychannels";
    }

    private List<RelayChannel> getRelayChannel()
    {
        List<RelayChannel> r;
        r = jdbcoperations.query("select serverid, userid, deviceid, userport, deviceport, userfd, devicefd, useraddress, deviceaddress, createdate from RelayChannels"
                , (rs, rownum) -> {
            RelayChannel rc = new RelayChannel();
                    rc.setServerid(rs.getInt("serverid"));
                    rc.setUserid(rs.getLong("userid"));
                    rc.setDeviceid(rs.getLong("deviceid"));
                    rc.setUserport(rs.getInt("userport"));
                    rc.setDevport(rs.getInt("deviceport"));
                    rc.setUserfd(rs.getInt("userfd"));
                    rc.setDevfd(rs.getInt("devicefd"));
                    rc.setUseraddress(rs.getString("useraddress"));
                    rc.setDevaddress(rs.getString("deviceaddress"));

                    java.util.Calendar cal = Calendar.getInstance();
                    cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
                    Timestamp ts = rs.getTimestamp("createdate", cal);
                    if (ts != null)
                    {
                        Date createtime = new Date(ts.getTime());
                        rc.setCreateDate(createtime);
                    }
            return rc;
        });
        return r;
    }
}
