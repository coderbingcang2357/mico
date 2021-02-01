package model;

import micoserver.MicoMessageSender;
import micoserver.message.CommandID;
import micoserver.message.Message;
import micoserver.message.MicoInternalMessage;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.dao.DataAccessException;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.jdbc.support.GeneratedKeyHolder;
import org.springframework.jdbc.support.KeyHolder;
import org.springframework.stereotype.Component;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.sql.*;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;

/**
 * Created by wqs on 8/25/17.
 */
@Component
public class PushMessageService {
    private final String getmsgsql = "select ID, createdate, message, shouldsend from PushMessage";
    private final String delmsgsql = "delete from PushMessage where ID=";
    private final String insertmsgsql = "insert into PushMessage (message, createdate) values (?, now())";
    private final String sendmsgsql = "update PushMessage set shouldsend=1 where ID=?";
    private final Logger log = LogManager.getLogger("micom");
    @Autowired
    private JdbcOperations jdbcoperations;

    @Autowired
    private MicoMessageSender micomessagesender;

    public List<PushMessge> getPushMessage() {
        List<PushMessge> pushmessages;
        pushmessages = jdbcoperations.query(getmsgsql, (rs, rownum)->{
            PushMessge pm = new PushMessge();
            pm.setId(rs.getLong("ID"));

            java.util.Calendar cal = Calendar.getInstance();
            cal.setTimeZone(TimeZone.getTimeZone("GMT+08"));
            Timestamp ts = rs.getTimestamp("createdate", cal);
            if (ts != null)
            {
                Date  lastlogintime = new Date(ts.getTime());
                pm.setCreatedate(lastlogintime);
            }

            pm.setMessage(rs.getString("message"));
            pm.setShouldsend(rs.getInt("shouldsend"));
            return pm;
        });

        return pushmessages;
    }

    public boolean deletePushMessage(long id) {
        String delquery = delmsgsql + String.valueOf(id);
        log.debug(delquery);
        try {
            int cnt = jdbcoperations.update(delquery);
            if (cnt <= 0)
                return false;
            else
                return true;
        }
        catch (DataAccessException e)
        {
            log.debug(e.toString());
        }
        return false;
    }

    public long insertPushMessage(String message) {
        KeyHolder holder = new GeneratedKeyHolder();
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(insertmsgsql, Statement.RETURN_GENERATED_KEYS);
                try {
                    ps.setString(1, message);
                } catch (Exception e) {
                    log.debug(e.toString());
                }
                return ps;
            }
        };
        try
        {
            int cnt = jdbcoperations.update(psc, holder);
            if (cnt <= 0)
            {
                return 0;
            }
            else
            {
                //jdbcoperations.
                return holder.getKey().longValue();
            }
        }
        catch (DataAccessException e)
        {
            log.debug(e.toString());
        }
        return 0;
    }

    public boolean sendPushMesssage(long msgid) {
        PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(sendmsgsql);
                try {
                    ps.setLong(1, msgid);
                } catch (Exception e) {
                    log.debug(e.toString());
                }
                return ps;
            }
        };
        try
        {
            int cnt = jdbcoperations.update(psc);
            if (cnt <= 0)
            {
                return false;
            }
            else
            {
                // send a notify to mico server
                sendNotifyToMicoServer(msgid);
                return true;
            }
        }
        catch (DataAccessException e)
        {
            log.debug(e.toString());
        }
        return false;
    }

    private void sendNotifyToMicoServer(long msgid) {
        String msg = String.format("{\"notifyid\":%d}", msgid);
        //MicoInternalMessage micointermessage = new MicoInternalMessage();
        //micointermessage.setType(CommandID.MESSAGE_TYPE_FROM_EXT_SERVER);
        //micointermessage.setSubtype(CommandID.MESSAGE_TYPE_UNUSED);
        //micointermessage.setVersion((short)0);
        //micointermessage.setEncryptMethod((byte)0);
        //Message msg = new Message();
        //msg.setPrefix(CommandID.CLIENT_MSG_PREFIX);
        //msg.setSuffix(CommandID.CLIENT_MSG_SUFFIX);
        //msg.setSerialNumber((short)0);
        //msg.setVersionNumber((short)0);
        //msg.setCommandID(CommandID.IN_CMD_NEW_PUSH_MESSAGE);
        //msg.setObjectID(0);
        //msg.setDestID(0);
        //ByteBuffer idlist = ByteBuffer.allocate(8);
        //idlist.order(ByteOrder.LITTLE_ENDIAN);
        //idlist.putLong(msgid);
        //msg.setContent(idlist.array());
        //micointermessage.setMessage(msg);
        micomessagesender.sendMessage(msg);
    }
}
