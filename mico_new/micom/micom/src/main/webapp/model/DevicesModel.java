package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.List;

/**
 * Created by wqs on 12/26/16.
 */
@Component
public class DevicesModel {
    private static String sql=" select ID,firmClusterID,name,macAddr,clusterID from T_Device order by ID desc limit ?, ?";
    @Autowired
    private JdbcOperations jdbcoperations;

    public util.Pair<List<DeviceInfo>, Integer> getDevs(int page, int size)
    {
        int count = getDevCount();
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
        List<DeviceInfo> devs;
        // select ID,firmClusterID,name,macAddr,clusterID
        devs = jdbcoperations.query(psc, (rs, rowNum)->{
            DeviceInfo dev = new DeviceInfo();
            dev.setId(rs.getLong("ID"));
            dev.setFirmClusterID(rs.getLong("firmClusterID"));
            dev.setName(rs.getString("name"));
            dev.setMacAddr(rs.getString("macAddr"));
            dev.setClusterID(rs.getLong("clusterID"));
            return dev;
        });
        return new util.Pair<List<DeviceInfo>, Integer>(devs, count);
    }

    int getDevCount()
    {
        Integer cnt;
        cnt = jdbcoperations.queryForObject("select count(*) from T_Device", Integer.class);
        return cnt.intValue();
    }
}
