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
 * Created by wqs on 9/8/16.
 */
@Component
public class SceneModel {
    private final  String FindSceneByIDStat =
            "select ID, name, creatorID, ownerID, createDate, description, version, scenesize, filelist, fileserverid from T_Scene where ID=?";
    private final String GetSceneSharedUserStat =
            "select T_User.ID, T_User.account, T_User_Scene.authority from T_User_Scene inner join T_User on T_User.ID=T_User_Scene.userID where T_User_Scene.sceneID=?";
    private final String GetSceneConnectionsStat =
            "select connectorid, deviceid from SceneConnector where sceneid=?";

    @Autowired
    private JdbcOperations jdbcoperations;

    public SceneModel() {}

    public List<SceneInfo> findSceneByID(long sceneid) {
        List<SceneInfo> ls;

        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement ps = connection.prepareStatement(FindSceneByIDStat);
                try {
                    ps.setLong(1, sceneid);
                }
                catch (Exception e) {
                    return null;
                }
                return ps;
            }
        };
        // ID, name, creatorID, ownerID, createDate, description, version, scenesize, filelist, fileserverid
        ls = jdbcoperations.query(psc, (rs, rownbr) -> {
            SceneInfo si = new SceneInfo();
            si.setID(rs.getLong("ID"));
            si.setName(rs.getString("name"));
            si.setCreatorID(rs.getLong("creatorID"));
            si.setOwnerID(rs.getLong("ownerID"));
            si.setCreateDate(rs.getDate("createDate"));
            si.setDescription(rs.getString("description"));
            si.setVersion(rs.getString("version"));
            si.setScenesize(rs.getInt("scenesize"));
            si.setFilelist(rs.getString("filelist"));
            si.setFileserverid(rs.getInt("fileserverid"));
            return si;
        });

        //
        for (SceneInfo si : ls) {
            getSceneSharedUser(si);
            getSceneConnections(si);
        }
        return ls;
    }

    public void getSceneSharedUser(SceneInfo si) {
        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement ps = connection.prepareStatement(GetSceneSharedUserStat);
                try {
                    ps.setLong(1, si.getID());
                }
                catch (Exception e) {
                    return null;
                }
                return ps;
            }
        };
        List<SceneSharedUser> shuers;
        shuers = jdbcoperations.query(psc, (rs, rownbr) -> {
            SceneSharedUser shuser = new SceneSharedUser();
            shuser.setId(rs.getLong("ID"));
            shuser.setUsername(rs.getString("account"));
            shuser.setAuth(rs.getInt("authority"));
            return shuser;
        });
        si.setSharedUsers(shuers);
    }

    private void getSceneConnections(SceneInfo si) {
        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection connection) throws SQLException {
                PreparedStatement ps  = connection.prepareStatement(GetSceneConnectionsStat);
                try {
                    ps.setLong(1, si.getID());
                }
                catch (Exception e) {
                    return null;
                }
                return ps;
            }
        };
        List<SceneConnection> sconns;
        sconns = jdbcoperations.query(psc, (rs, rownbr) -> {
            SceneConnection sc = new SceneConnection();
            sc.setId(rs.getInt("connectorid"));
            sc.setDeviceID(rs.getLong("deviceid"));
            return sc;
        });
        si.setConnections(sconns);
    }
}
