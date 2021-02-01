package model;

/**
 * Created by wqs on 9/8/16.
 * 对设备有权限的人
 */
public class DeviceUser {
    private long userid;
    private String username;
    private int auth;

    public long getUserid() {
        return userid;
    }

    public void setUserid(long userid) {
        this.userid = userid;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public int getAuth() {
        return auth;
    }

    public void setAuth(int auth) {
        this.auth = auth;
    }
}
