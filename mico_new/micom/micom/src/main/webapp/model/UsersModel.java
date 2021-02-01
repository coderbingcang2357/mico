package model;

import org.apache.logging.log4j.core.util.ObjectArrayIterator;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessException;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.jdbc.support.GeneratedKeyHolder;
import org.springframework.stereotype.Component;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;
/**
 * Created by wqs on 12/22/16.
 */
@Component
public class UsersModel {
    private final Logger log = LogManager.getLogger("micom");
    private static String sql="select ID, account, password, mail, phone, signature, lastLoginTime, createDate from T_User order by ID desc limit ?, ?";
    @Autowired
    private JdbcOperations jdbcoperations;

    public UsersModel() {
    }

    public util.Pair<List<UserInfo>, Integer> getUsers(int page, int size)
    {
        int count = getUserCount();
        int startpos = page * size;
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(sql);
                try {
                    ps.setLong(1, startpos);
                    ps.setLong(2, size);
                } catch (Exception e) {
                    log.debug("UsersModel.getUsers" + e.toString());
                }
                return ps;
            }
        };
        List<UserInfo> users;
        // select (select count(*) from T_User) as "count", ID, account, password, mail, phone, signature, lastLoginTime, createDate
        users = jdbcoperations.query(psc, (rs, rowNum)-> {
            UserInfo usr = new UserInfo();
            usr.setId(rs.getLong("ID"));
            usr.setAccount(rs.getString("account"));
            usr.setPassword(rs.getString("password"));
            usr.setMail(rs.getString("mail"));
            usr.setSignature(rs.getString("signature"));
            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            usr.setCreateDate(new Date(rs.getTimestamp("createDate", cal).getTime()));
            return usr;
        });
        return new util.Pair<List<UserInfo>, Integer>(users, count);
    }

    int getUserCount()
    {
        Integer cnt;
        cnt = jdbcoperations.queryForObject("select count(*) from T_User", Integer.class);
        return cnt.intValue();
    }

    public boolean modifyUserPassword(long userid, String newpwd)
    {
        if (newpwd.length() <= 0)
            return false;
        String md5pwd = util.Md5.get(newpwd.trim());

        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement("update T_User set password=? where ID=?");
                try {
                    ps.setString(1, md5pwd);
                    ps.setLong(2, userid);
                } catch (Exception e) {
                    String es = e.toString();
                    log.debug("UsersModel.modify pwd: " + es);
                }
                return ps;
            }
        };
        int cnt = 0;
        try {
            cnt = jdbcoperations.update(psc);
        } catch (DataAccessException e) {
            // write log
            String err = e.toString();
            log.debug("UsersModel.modify pwd2: " + err);
        }
        return cnt > 0;
    }

    public boolean modifyUserMail(long userid, String newmail)
    {
        String trimnewmail = newmail.trim();
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement("update T_User set mail=? where ID=?");
                try {
                    ps.setString(1, trimnewmail);
                    ps.setLong(2, userid);
                } catch (Exception e) {
                    String es = e.toString();
                    log.debug("UsersModel.modify mail: " + es);
                }
                return ps;
            }
        };
        int cnt = 0;
        try {
            cnt = jdbcoperations.update(psc);
        } catch (DataAccessException e) {
            // write log
            String err = e.toString();
            log.debug("UsersModel.modify mail2: " + err);
        }
        return cnt > 0;
    }

    public boolean deleteDeviceOfUser(long userid, long deviceid) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement("delete from T_User_Device where userID=? and deviceID=?");
                try {
                    ps.setLong(1, userid);
                    ps.setLong(2, deviceid);
                } catch (Exception e) {
                    log.debug("UsersModel.deleteDeviceOfUser bind " + e.toString());
                }
                return ps;
            }
        };
        int cnt = 0;
        try {
            cnt = jdbcoperations.update(psc);
        } catch (DataAccessException e) {
            // write log
            String err = e.toString();
            log.debug("UsersModel.deleteDeviceOfUser dataaccess exception " + e.toString());
        }
        return cnt > 0;
    }

    public Object [] createCluster(long userid, String clusteraccount, String desc) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(
                        "insert into T_DeviceCluster (type, account, describ, creatorID, sysAdminID, createDate) values (0, ?, ?, ?, ?, now())",
                        Statement.RETURN_GENERATED_KEYS);
                try {
                    ps.setString(1, clusteraccount);
                    ps.setString(2, desc);
                    ps.setLong(3, userid);
                    ps.setLong(4, userid);
                } catch (Exception e) {
                    log.debug("UsersModel.createCluster bind " + e.toString());
                }
                return ps;
            }
        };
        int cnt = 0;
        int resultadduser = 0;
        long clusterid = 0;
        try {
            jdbcoperations.update("begin");
            // create cluster
            GeneratedKeyHolder clusteridholder = new GeneratedKeyHolder();
            cnt = jdbcoperations.update(psc, clusteridholder);
            // get clusterid
            long tmpclusterid = clusteridholder.getKey().longValue();
            clusterid = tmpclusterid;
            // addd creator to the cluster
            PreparedStatementCreator addcreatortocluster = new PreparedStatementCreator() {
                @Override
                public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                    PreparedStatement ps = conn.prepareStatement(
                            "insert into T_User_DevCluster (userID, deviceClusterID, role, deleteFlag) values (?,?,?, 0)");
                    try {
                        ps.setLong(1, userid);
                        ps.setLong(2, tmpclusterid);
                        ps.setInt(3, 1); // role 1 means Manager
                    } catch (Exception e) {
                        log.debug("UsersModel.createCluster add user to cluster bind" + e.toString());
                    }
                    return ps;
                }
            };
            resultadduser = jdbcoperations.update(addcreatortocluster);
            jdbcoperations.update("commit");
        } catch (DataAccessException e) {
            // write log
            String err = e.toString();
            log.debug("UsersModel.createCluster dataaccess exception " + e.toString());
            jdbcoperations.update("rollback");
            cnt = 0; // failure flag
        }
        Object []res = new Object[2];
        Boolean b = new Boolean(cnt > 0 && resultadduser > 0);
        Long id = new Long(clusterid);
        res[0] = b;
        res[1] = id;
        return res;
    }
}
