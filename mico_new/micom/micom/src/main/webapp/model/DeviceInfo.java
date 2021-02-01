package model;

import java.util.Date;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
public class DeviceInfo {
    private long id;
    private String name;
    private long firmClusterID;
    private String macAddr;
    private long clusterID;
    private String loginKey;
    private Date lastOnlineTime;
    private int onlinetime;
    private List<DeviceUser> deviceusers;


    public Date getLastOnlineTime() {
        return lastOnlineTime;
    }

    public void setLastOnlineTime(Date lastOnlineTime) {
        this.lastOnlineTime = lastOnlineTime;
    }

    public int getOnlinetime() {
        return onlinetime;
    }

    public void setOnlinetime(int onlinetime) {
        this.onlinetime = onlinetime;
    }

    public List<DeviceUser> getDeviceusers() {
        return deviceusers;
    }

    public void setDeviceusers(List<DeviceUser> deviceusers) {
        this.deviceusers = deviceusers;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public long getFirmClusterID() {
        return firmClusterID;
    }

    public void setFirmClusterID(long firmClusterID) {
        this.firmClusterID = firmClusterID;
    }

    public String getMacAddr() {
        return macAddr;
    }

    public void setMacAddr(String macAddr) {
        this.macAddr = macAddr;
    }

    public long getClusterID() {
        return clusterID;
    }

    public void setClusterID(long clusterID) {
        this.clusterID = clusterID;
    }

    public String getLoginKey() {
        return loginKey;
    }

    public void setLoginKey(String loginKey) {
        this.loginKey = loginKey;
    }
}
