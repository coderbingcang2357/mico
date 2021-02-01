package model;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.jdbc.core.JdbcOperations;
import org.springframework.jdbc.core.RowMapper;
import org.springframework.stereotype.Component;
import util.Pair;

import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Created by wqs on 12/28/16.
 */
@Component
public class CommonSearchModel {
    @Autowired
    private JdbcOperations jdbcoperations;
    private AtomicBoolean hasupdate = new AtomicBoolean(false);
    private HashMap<String, List<IndexItem>> index = new HashMap<String, List<IndexItem>>();
    public CommonSearchModel() {
    }

    public List<IndexItem> get(String texttosearch, boolean shouldupdteindex) {
        if (shouldupdteindex || hasupdate.compareAndSet(false, true)) {
            updateIndex();
        }

        synchronized (this) {
            return index.get(texttosearch);
        }
    }

    private void updateIndex() {
        HashMap<String, List<IndexItem>> newidx = new HashMap<String, List<IndexItem>>();
        readUser(newidx);
        readDevice(newidx);
        readCluster(newidx);
        synchronized (this) {
            index = newidx;
        }
        //readScene();
    }

    private void readUser(HashMap<String, List<IndexItem>> newidx) {
        jdbcoperations.query("select ID, account from T_User",
                new UserRowMapper(newidx, "User", "ID", "account"));
    }

    private void readDevice(HashMap<String, List<IndexItem>> newidx) {
        jdbcoperations.query("select ID, name from T_Device",
                new UserRowMapper(newidx, "Device", "ID", "name"));
    }

    private void readCluster(HashMap<String, List<IndexItem>> newidx) {
        jdbcoperations.query("select ID, account from T_DeviceCluster",
                new UserRowMapper(newidx, "Cluster", "ID", "account"));
    }

    public final static class IndexItem {
        private String type;
        private long id;
        private String text;

        IndexItem(String t, long id, String txt) {
            this.type = t;
            this.id = id;
            this.text = txt;
        }

        public String getType() {
            return type;
        }

        public void setType(String type) {
            this.type = type;
        }

        public long getId() {
            return id;
        }

        public void setId(long id) {
            this.id = id;
        }

        public String getText() {
            return text;
        }

        public void setText(String text) {
            this.text = text;
        }
    }

    private final static class UserRowMapper implements RowMapper<IndexItem> {
        private  HashMap<String, List<IndexItem>> newidx;
        private String type;
        private  String columnID;
        private String columnName;
        UserRowMapper(HashMap<String, List<IndexItem>> newidx,
                             String type,
                             String id,
                             String name) {
            this.newidx = newidx;
            this.type = type;
            this.columnID = id;
            this.columnName = name;
        }
        public IndexItem mapRow(ResultSet rs, int rownum) {
            try {
                long id = rs.getLong(columnID);
                String account = rs.getString(columnName);
                if (account != null)
                    insert(newidx, type, id, account.toLowerCase());
            } catch (SQLException e) {

            }
            return null;
        }

        private void insert(HashMap<String, List<IndexItem>> newidx,
                            String type,
                            long id,
                            String account) {
            HashSet<String> added = new HashSet<String>();
            for (int i = 0; i < account.length(); ++i)
            {
                for (int j = i; j < account.length(); ++j)
                {
                    String tmp = account.substring(i, j+1);
                    if (added.contains(tmp))
                    {
                        continue;
                    }
                    added.add(tmp);
                    List<IndexItem> vl = newidx.get(tmp);
                    if (vl == null) {
                        vl = new ArrayList<IndexItem>();
                        newidx.put(tmp, vl);
                    }
                    vl.add( new IndexItem(type, id, account));
                }
            }
        }
    }
}
