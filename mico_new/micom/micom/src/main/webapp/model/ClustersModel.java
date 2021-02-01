package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessException;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by wqs on 12/26/16.
 */
@Component
public class ClustersModel {
    private static String sql="select ID,account,describ,creatorID,sysAdminID,createDate from T_DeviceCluster order by ID desc limit ?,?";
    @Autowired
    private JdbcOperations jdbcoperations;

    public util.Pair<List<ClusterInfo>, Integer> getClusters(int page, int size)
    {
        int count = getDevClusterCount();
        int startpos = page * size;
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(sql);
                try {
                    ps.setLong(1, startpos);
                    ps.setLong(2, size);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<ClusterInfo> clus;
        // select ID,account,describ,creatorID,sysAdminID,createDate
        clus = jdbcoperations.query(psc, (rs, rowNum)->{
            ClusterInfo clu = new ClusterInfo();
            clu.setId(rs.getLong("ID"));
            clu.setAccount(rs.getString("account"));
            clu.setDescrib(rs.getString("describ"));
            clu.setCreatorID(rs.getLong("creatorID"));
            clu.setSysAdminID(rs.getLong("sysAdminID"));
            clu.setCreateDate(rs.getDate("createDate"));
            return clu;
        });
        return new util.Pair<List<ClusterInfo>, Integer>(clus, count);
    }

    int getDevClusterCount()
    {
        Integer cnt;
        cnt = jdbcoperations.queryForObject("select count(*) from T_DeviceCluster", Integer.class);
        return cnt.intValue();
    }

    public String deviceAuthorize(long adminid, long deviceid, ArrayList<Long> userid) {
        StringBuilder sql = new StringBuilder();
        sql.append("insert into T_User_Device (userID, deviceID, authorizerID, authority) values ");
        for (int idx = 0; idx < userid.size(); ++idx) {
            long id = userid.get(idx);
            sql.append("(");
            sql.append(id);
            sql.append(",");
            sql.append(deviceid);
            sql.append(",");
            sql.append(adminid);
            sql.append(",");
            sql.append(2);
            sql.append(")");
            // is not last then append a ','
            if (idx != (userid.size() - 1))
            {
                sql.append(",");
            }
        }
        String res  = new String("success");
        try {
            int cnt = jdbcoperations.update(sql.toString());
            if (cnt <= 0)
            {
                res = new String("failure");
            }
        }catch (DataAccessException e) {
            res = e.toString();
        }

        return res;
    }

    public String addUserToCluster(long clusterid, ArrayList<Long> userids) {
        String res = "success";
        StringBuilder sql = new StringBuilder();
        sql.append("insert into T_User_DevCluster (userID, deviceClusterID, role, deleteFlag) values ");
        for (int i = 0; i < userids.size(); ++i) {
            sql.append("(");
            sql.append(userids.get(i));
            sql.append(",");

            sql.append(clusterid);
            sql.append(",");

            sql.append(3); // 3表示操作员
            sql.append(",");

            sql.append(0); // delete flag 0

            sql.append(")");
            if (i != (userids.size() - 1)) {
                sql.append(",");
            }
        }
        try {
            int cnt = jdbcoperations.update(sql.toString());
            if (cnt <= 0) {
                res = new String("failure");
            }
        } catch (DataAccessException e) {
            res = e.toString();
        }

        return res;
    }
}
