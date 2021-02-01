package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
@Component
public class SearchClusterModel {
    private final static String FindByIDStat =
    "select ID, account, notename, describ, creatorID, sysAdminID, createDate from T_DeviceCluster where ID=?";
    private final static String FindByNameStat =
    "select ID, account, notename, describ, creatorID, sysAdminID, createDate from T_DeviceCluster where account=?";
    private final  static String GetClusterUsersStat =
            "select userID, role, account from T_User_DevCluster join T_User on T_User.ID=T_User_DevCluster.userID where deviceClusterID=?";
    private final static String GetClusterDeviceStat =
            "select ID, name from T_Device where clusterID=?";

    @Autowired
    private JdbcOperations jdbcoperations;

    public List<ClusterInfo> searchClusterByName(String name) {

        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindByNameStat);
                try {
                    ps.setString(1, name);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<ClusterInfo> lc;
        // "select ID, account, notename, describ, creatorID, sysAdminID, createDate from T_DeviceCluster where account=?";
        lc = jdbcoperations.query(psc, (rs, rowNum)->{
            ClusterInfo c = new ClusterInfo();
            c.setId(rs.getLong("ID"));
            c.setAccount(rs.getString("account"));
            c.setNotename(rs.getString("notename"));
            c.setDescrib(rs.getString("describ"));
            c.setCreatorID(rs.getLong("creatorID"));
            c.setSysAdminID(rs.getLong("sysAdminID"));
            c.setCreateDate(rs.getDate("createDate"));
            return c;
        });

        for (ClusterInfo ci : lc) {
            getClusterUsers(ci);
            getClusterDevice(ci);
        }

        return lc;
    }

    public List<ClusterInfo> searchClusterByID(long id) {
        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FindByIDStat);
                try {
                    ps.setLong(1, id);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<ClusterInfo> lc;
        // "select ID, account, notename, describ, creatorID, sysAdminID, createDate from T_DeviceCluster where account=?";
        lc = jdbcoperations.query(psc, (rs, rowNum)->{
            ClusterInfo c = new ClusterInfo();
            c.setId(rs.getLong("ID"));
            c.setAccount(rs.getString("account"));
            c.setNotename(rs.getString("notename"));
            c.setDescrib(rs.getString("describ"));
            c.setCreatorID(rs.getLong("creatorID"));
            c.setSysAdminID(rs.getLong("sysAdminID"));
            c.setCreateDate(rs.getDate("createDate"));
            return c;
        });

        for (ClusterInfo ci : lc) {
            getClusterUsers(ci);
            getClusterDevice(ci);
        }

        return lc;
    }

    public void getClusterUsers(ClusterInfo ci) {
        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement pc = connection.prepareStatement(GetClusterUsersStat);
                try {
                    pc.setLong(1, ci.getId());
                }
                catch (Exception e) {
                    return null;
                }
                return pc;
            }
        };
        // userID, role, account
        List<ClusterUser> cusers;
        cusers = jdbcoperations.query(psc, (rs, rownbr) -> {
            ClusterUser cu = new ClusterUser();
            cu.setId(rs.getLong("userID"));
            cu.setName(rs.getString("account"));
            cu.setRole(rs.getInt("role"));
            return cu;
        });

        ci.setUsers(cusers);
    }

    public void getClusterDevice(ClusterInfo ci) {
        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement ps = connection.prepareCall(GetClusterDeviceStat);
                try {
                    ps.setLong(1, ci.getId());
                }
                catch (Exception e) {
                    return null;
                }
                return ps;
            }
        };
        List<ClusterDevice> devs;
        // ID, name
        devs = jdbcoperations.query(psc, (rs, rownbr)->{
            ClusterDevice cd = new ClusterDevice();
            cd.setId(rs.getLong("ID"));
            cd.setName(rs.getString("name"));
            return cd;
        });

        ci.setDevices(devs);
    }
}
