package model;

import java.util.Date;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
public class ClusterInfo {
    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getAccount() {
        return account;
    }

    public void setAccount(String account) {
        this.account = account;
    }

    public String getNotename() {
        return notename;
    }

    public void setNotename(String notename) {
        this.notename = notename;
    }

    public String getDescrib() {
        return describ;
    }

    public void setDescrib(String describ) {
        this.describ = describ;
    }

    public long getCreatorID() {
        return creatorID;
    }

    public void setCreatorID(long creatorID) {
        this.creatorID = creatorID;
    }

    public long getSysAdminID() {
        return sysAdminID;
    }

    public void setSysAdminID(long sysAdminID) {
        this.sysAdminID = sysAdminID;
    }

    public Date getCreateDate() {
        return createDate;
    }

    public void setCreateDate(Date createDate) {
        this.createDate = createDate;
    }

    public List<ClusterUser> getUsers() {
        return users;
    }

    public void setUsers(List<ClusterUser> users) {
        this.users = users;
    }

    public List<ClusterDevice> getDevices() {
        return devices;
    }

    public void setDevices(List<ClusterDevice> devices) {
        this.devices = devices;
    }

    private long id;
    private String account;
    private String notename;
    private String describ;
    private long creatorID;
    private long sysAdminID;
    private Date createDate;
    private List<ClusterUser> users;
    private List<ClusterDevice> devices;
}
