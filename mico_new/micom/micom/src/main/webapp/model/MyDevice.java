package model;

/**
 * Created by wqs on 8/1/16.
 */
public class MyDevice {
    public long getDeviceid() {
        return deviceid;
    }

    public void setDeviceid(long deviceid) {
        this.deviceid = deviceid;
    }

    public int getAuth() {
        return auth;
    }

    public void setAuth(int auth) {
        this.auth = auth;
    }

    public long getAuthid() {
        return authid;
    }

    public void setAuthid(long autoid) {
        this.authid = autoid;
    }

    public String getDeviceName() {
        return deviceName;
    }

    public void setDeviceName(String devceName) {
        this.deviceName = devceName;
    }

    private long deviceid;
    private int auth;
    private long authid;
    private String deviceName;
}
