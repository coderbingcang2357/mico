package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;

import java.sql.*;
import java.util.*;
import java.util.Date;

/**
 * Created by wqs on 7/27/16.
 */
@Component
public class SearchDeviceModel {

    private final static String FindDeviceByNameStat =
            "select ID, name, firmClusterID, macAddr, clusterID, lastOnlineTime, onlinetime, szLoginKey from T_Device where name=?";
    private final static String FindDeviceByIDStat =
            "select ID, name, firmClusterID, macAddr, clusterID, lastOnlineTime, onlinetime, szLoginKey from T_Device where ID=?";
    private final  static String GetDeviceUserStat =
            "select userID, T_User.account, authority from T_User_Device join T_User on userID=T_User.ID where deviceID=?";

    @Autowired
    private JdbcOperations jdbcoperations;

    public List<DeviceInfo> searchDeviceByName(String name) {

        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindDeviceByNameStat);
                try {
                    ps.setString(1, name);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<DeviceInfo> ld;
        // "select ID, name, firmClusterID, macAddr, clusterID, szLoginKey from T_Device where name=?";
        ld = jdbcoperations.query(psc, (rs, rowNum)->{
            DeviceInfo d = new DeviceInfo();
            d.setId(rs.getLong("ID"));
            d.setName(rs.getString("name"));
            d.setFirmClusterID(rs.getLong("firmClusterID"));
            Long macAddrLong = rs.getLong("macAddr");
            d.setMacAddr(Long.toHexString(macAddrLong));
            d.setClusterID(rs.getLong("clusterID"));
            d.setLoginKey(rs.getString("szLoginKey"));
            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            Date lastlogintime = rs.getTimestamp("lastOnlineTime", cal);
            if (lastlogintime != null)
            {
                d.setLastOnlineTime(lastlogintime);
            }
            d.setOnlinetime(rs.getInt("onlinetime"));
            return d;
        });

        // 取设备的所有使用者
        for (DeviceInfo di : ld) {
            getDeviceUsers(di);
        }

        return ld;
    }

    public List<DeviceInfo> searchDeviceByID(long id) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindDeviceByIDStat);
                try {
                    ps.setLong(1, id);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<DeviceInfo> ld;
        // "select ID, name, firmClusterID, macAddr, clusterID, szLoginKey from T_Device where name=?";
        ld = jdbcoperations.query(psc, (rs, rowNum)->{
            DeviceInfo d = new DeviceInfo();
            d.setId(rs.getLong("ID"));
            d.setName(rs.getString("name"));
            d.setFirmClusterID(rs.getLong("firmClusterID"));
            Long macAddrLong = rs.getLong("macAddr");
            d.setMacAddr(Long.toHexString(macAddrLong));
            d.setClusterID(rs.getLong("clusterID"));
            d.setLoginKey(rs.getString("szLoginKey"));
            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            Timestamp ts = rs.getTimestamp("lastOnlineTime", cal);
            if (ts != null)
            {
                Date lastlogintime = new Date(ts.getTime());
                d.setLastOnlineTime(lastlogintime);
            }
            d.setOnlinetime(rs.getInt("onlinetime"));
            return d;
        });

        // 取设备的所有使用者
        for (DeviceInfo di : ld) {
            getDeviceUsers(di);
        }
        return ld;
    }

    void getDeviceUsers(DeviceInfo devinfo) {

        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement ps = connection.prepareStatement(GetDeviceUserStat);
                try {
                    ps.setLong(1, devinfo.getId());
                }
                catch (Exception e) {
                    return null;
                }
                return ps;
            }
        };

        List<DeviceUser> devusers;
        devusers = jdbcoperations.query(psc, (rs, rownbr) -> {
            DeviceUser du = new DeviceUser();
            du.setUsername(rs.getString("account"));
            du.setUserid(rs.getLong("userID"));
            du.setAuth(rs.getInt("authority"));
            return du;
        });

        devinfo.setDeviceusers(devusers);
        return;
    }
}
