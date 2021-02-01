package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.PreparedStatementCreator;
import org.springframework.stereotype.Component;
import util.Pair;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
@Component
public class FeedBackModel {

    private static final String FeedbackStat =
            "select ID, userID, feedback, submitTime from T_Feedback where ID >= ? and ID <= ? order by ID desc;";
    private static final String GetFeedBackCount = "select count(*) as sum from T_Feedback";

    @Autowired
    private JdbcOperations jdbcop;

    public util.Pair<Integer, List<FeedbackItem>> getFeedBack(int page, int pagesize) {
        int all = getFeedBackCount();
        int end = all - page * pagesize;
        int start = all - (page + 1) * pagesize;
        start = start < 0 ? 0 : start;

        final int startfinal = start;

        final PreparedStatementCreator psc = new PreparedStatementCreator() {
            @Override
            public PreparedStatement createPreparedStatement(Connection conn) throws SQLException {
                PreparedStatement ps = conn.prepareStatement(FeedbackStat);
                try {
                    ps.setInt(1, startfinal);
                    ps.setInt(2, end);
                } catch (Exception e) {
                }
                return ps;
            }
        };
        List<FeedbackItem> lf;
        // "select ID, userID, feedback, submitTime from T_Feedback where ID >= ? and ID <= ? order by ID desc;";
        lf = jdbcop.query(psc, (rs, rowNum)->{
            FeedbackItem f = new FeedbackItem();
            f.setId(rs.getLong("ID"));
            f.setUserid(rs.getLong("userID"));
            f.setContent(rs.getString("feedback"));
            f.setDate(rs.getDate("submitTime"));
            return f;
        });

        return new Pair<Integer, List<FeedbackItem>>(new Integer(all), lf);
    }

    private int getFeedBackCount(){
        //int count = jdbcop.query(GetFeedBackCount, (rs, rowNum)->{
        //    return rs.getInt("sum");
        //});
        Integer cnt;
        cnt = jdbcop.queryForObject(GetFeedBackCount, Integer.class);
        return cnt.intValue();
    }
}
