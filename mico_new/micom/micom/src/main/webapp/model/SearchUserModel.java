package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.autoconfigure.security.SecurityProperties;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.util.*;

/**
 * Created by wqs on 7/26/16.
 */
@Component
public class SearchUserModel {
    private static final String FindUserByNameStat=
            "select ID, account, password, mail, signature, head, lastLoginTime, onlinetime, createDate from T_User where account=?";
    private static final String FindUserByIDStat=
            "select ID, account, password, mail, signature, head, lastLoginTime, onlinetime, createDate from T_User where ID=?";
    private static final String GetMyDeviceStat =
            "select T_User_Device.userID, T_User_Device.deviceID, T_User_Device.authority, T_User_Device.authorizerID, T_Device.name from T_User_Device join T_Device on T_User_Device.deviceID=T_Device.ID where userID=?";
    private static final String GetUserCluster =
            "select deviceClusterID, role, notename from T_User_DevCluster where userID=?;";
    private static final String GetUserScene =
            "select sceneID,authority,notename from T_User_Scene where userID=?";
    @Autowired
    private JdbcOperations jdbcoperations;

    public List<User> searchUser(String username)
    {
        return getUserByName(username);
    }

    public List<User> searchUser(Long userid)
    {
        return getUserById(userid);
    }

    private List<MyDevice> getUserDevice (Long id) {
        List<MyDevice> md;
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(GetMyDeviceStat);
                try {
                    ps.setLong(1, id);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        // "select T_User_Device.userID, T_User_Device.deviceID, T_User_Device.authority,
        // T_User_Device.authorizerID, T_Device.name
        md = jdbcoperations.query(psc, (rs, rowNum)->{
            MyDevice myd = new MyDevice();
            myd.setDeviceid(rs.getLong("deviceID"));
            myd.setDeviceName(rs.getString("name"));
            myd.setAuth(rs.getInt("authority"));
            myd.setAuthid(rs.getLong("authorizerID"));
            return myd;
        });
        return md;
    }

    private List<MyCluster> getUserCluster(Long id) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(GetUserCluster);
                try {
                    ps.setLong(1, id);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<MyCluster> lm;
        // "select deviceClusterID, role, notename from T_User_DevCluster where userID=?;";
        lm = jdbcoperations.query(psc, (rs, rowNum)->{
            MyCluster myc = new MyCluster();
            myc.setId(rs.getLong("deviceClusterID"));
            myc.setRole(rs.getInt("role"));
            myc.setNotename(rs.getString("notename"));
            return myc;
        });
        return lm;
    }

    private List<MyScene> getUserScene(Long id) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(GetUserScene);
                try {
                    ps.setLong(1, id);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<MyScene> ls;
        // "select sceneID,authority,notename from T_User_Scene where userID=?";
        ls = jdbcoperations.query(psc, (rs, rowNum)->{
            MyScene mys = new MyScene();
            mys.setId(rs.getLong("sceneID"));
            mys.setRole(rs.getInt("authority"));
            mys.setNotename(rs.getString("notename"));
            return mys;
        });
        return ls;
    }

    private List<User> getUserByName(String username) {

        List<UserInfo> ul;
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindUserByNameStat);
                try {
                    ps.setString(1, username);
                } catch (Exception e) {
                }
                return ps;
            }
        };

        // "select ID, account, password, mail, signature, head, createDate from T_User where account=?";
        ul = jdbcoperations.query(psc, (rs, rowNum)->{
            UserInfo u = new UserInfo();
            u.setId(rs.getLong("ID"));
            u.setAccount(rs.getString("account"));
            u.setPassword(rs.getString("password"));
            u.setMail(rs.getString("mail"));
            u.setSignature(rs.getString("signature"));
            u.setHead(rs.getInt("head"));
            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            //usr.setCreateDate(new Date(rs.getTimestamp("createDate", cal).getTime()));
            Timestamp lastlogintime = rs.getTimestamp("lastLoginTime", cal);
            if (lastlogintime != null)
            {
                u.setLastloginTime(lastlogintime);
            }
            u.setOnlinetimes(rs.getInt("onlinetime"));
            u.setCreateDate(rs.getDate("createDate"));
            return u;
        });

        List<User> users = new ArrayList<User>();
        for (UserInfo ui : ul) {
            User user = new User();
            user.setUserinfo(ui);
            user.setDevices(this.getUserDevice(ui.getId()));
            user.setClusters(this.getUserCluster(ui.getId()));
            user.setScenes(this.getUserScene(ui.getId()));
            users.add(user);
        }

        // ul = getUserDevice(ul);
        // ul = getUserCluster(ul);
        // ul = getUserScene(ul);
        return users;
    }

    private List<User> getUserById(long userid) {
        List<UserInfo> ul;
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindUserByIDStat);
                try {
                    ps.setLong(1, userid);
                } catch (Exception e) {
                }
                return ps;
            }
        };

        // "select ID, account, password, mail, signature, head, lastLoginTime, onlinetime, createDate from T_User where account=?";
        ul = jdbcoperations.query(psc, (rs, rowNum)->{
            UserInfo u = new UserInfo();
            u.setId(rs.getLong("ID"));
            u.setAccount(rs.getString("account"));
            u.setPassword(rs.getString("password"));
            u.setMail(rs.getString("mail"));
            u.setSignature(rs.getString("signature"));
            u.setHead(rs.getInt("head"));
            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            //usr.setCreateDate(new Date(rs.getTimestamp("createDate", cal).getTime()));
            Timestamp lastlogintime = rs.getTimestamp("lastLoginTime", cal);
            if (lastlogintime != null)
            {
                u.setLastloginTime(lastlogintime);
            }
            u.setOnlinetimes(rs.getInt("onlinetime"));
            u.setCreateDate(rs.getTimestamp("createDate", cal));
            return u;
        });

        List<User> users = new ArrayList<User>();
        for (UserInfo ui : ul) {
            User user = new User();
            user.setUserinfo(ui);
            user.setDevices(this.getUserDevice(ui.getId()));
            user.setClusters(this.getUserCluster(ui.getId()));
            user.setScenes(this.getUserScene(ui.getId()));
            users.add(user);
        }

        // ul = getUserDevice(ul);
        // ul = getUserCluster(ul);
        // ul = getUserScene(ul);
        return users;
    }
}
