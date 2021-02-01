package model;

import java.util.Date;

/**
 * Created by wqs on 10/31/18.
 */
public class RelayChannel {
    private int serverid;
    private long userid;
    private long deviceid;
    private int userport;
    private int devport;
    private int userfd;
    private int devfd;
    private String useraddress;
    private String devaddress;
    private Date createDate;

    public int getServerid() {
        return serverid;
    }

    public void setServerid(int serverid) {
        this.serverid = serverid;
    }

    public long getUserid() {
        return userid;
    }

    public void setUserid(long userid) {
        this.userid = userid;
    }

    public long getDeviceid() {
        return deviceid;
    }

    public void setDeviceid(long deviceid) {
        this.deviceid = deviceid;
    }

    public int getUserport() {
        return userport;
    }

    public void setUserport(int userport) {
        this.userport = userport;
    }

    public int getDevport() {
        return devport;
    }

    public void setDevport(int devport) {
        this.devport = devport;
    }

    public int getUserfd() {
        return userfd;
    }

    public void setUserfd(int userfd) {
        this.userfd = userfd;
    }

    public int getDevfd() {
        return devfd;
    }

    public void setDevfd(int devfd) {
        this.devfd = devfd;
    }

    public String getUseraddress() {
        return useraddress;
    }

    public void setUseraddress(String useraddress) {
        this.useraddress = useraddress;
    }

    public String getDevaddress() {
        return devaddress;
    }

    public void setDevaddress(String devaddress) {
        this.devaddress = devaddress;
    }

    public Date getCreateDate() {
        return createDate;
    }

    public void setCreateDate(Date createDate) {
        this.createDate = createDate;
    }
}
