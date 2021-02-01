package model;

import java.sql.Timestamp;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;

/**
 * Created by wqs on 7/26/16.
 */
public class UserInfo {
    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public String getAccount() {
        return account;
    }

    public void setAccount(String account) {
        this.account = account;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getMail() {
        return mail;
    }

    public void setMail(String mail) {
        this.mail = mail;
    }

    public String getSignature() {
        return signature;
    }

    public void setSignature(String signature) {
        this.signature = signature;
    }

    public int getHead() {
        return head;
    }

    public void setHead(int head) {
        this.head = head;
    }

    public Date getCreateDate() {
        return createDate;
    }

    public void setCreateDate(Date createDate) {
        //createDateStr = createDate.toString();
        this.createDate = createDate;
    }

    //public List<ClusterInfo> getClusterlist() {
    //    return clusterlist;
    //}

    //public void setClusterlist(List<ClusterInfo> clusterlist) {
    //    this.clusterlist = clusterlist;
    //}

    //public List<DeviceInfo> getMydevicelist() {
    //    return mydevicelist;
    //}

    //public void setMydevicelist(List<DeviceInfo> mydevicelist) {
    //    this.mydevicelist = mydevicelist;
    //}

    //public List<SceneInfo> getMyscenelist() {
    //    return myscenelist;
    //}

    //public void setMyscenelist(List<SceneInfo> myscenelist) {
    //    this.myscenelist = myscenelist;
    //}

    private Long id;
    private String account;
    private String password;
    private String mail;
    private String signature;
    private int head;

    public Date getLastloginTime() {
        return lastloginTime;
    }

    public void setLastloginTime(Date lastloginTime) {
        this.lastloginTime = lastloginTime;
    }

    public int getOnlinetimes() {
        return onlinetimes;
    }

    public void setOnlinetimes(int onlinetimes) {
        this.onlinetimes = onlinetimes;
    }

    private Date lastloginTime;
    private int onlinetimes;
    private Date createDate;
    //private String createDateStr;

    public String getCreateDateStr() {
        DateFormat df = new SimpleDateFormat("MM/dd/yyyy HH:mm:ss");
        return df.format(createDate);
    }

    //private String createDateStr;
    //private List<ClusterInfo> clusterlist;
    //private List<DeviceInfo> mydevicelist;
    //private List<SceneInfo> myscenelist;
    // clusterlist
    // mydevicelist
    // scenelist
}
