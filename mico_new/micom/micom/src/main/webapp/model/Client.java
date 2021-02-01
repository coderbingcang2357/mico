package model;

import java.util.Date;

/**
 * Created by wqs on 7/21/16.
 * Client 用于表示一个在线的客户端,　包括Ｍico客户端和设备
 */
public class Client {
    public Client() {
        this(0, "", new Date());
    }

    public Client(long i, String n, Date t) {
        this.id = i;
        this.name = n;
        this.time = t;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }


    //
    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }


    public Date getTime() {
        return time;
    }

    public void setTime(Date time) {
        this.time = time;
    }


    //
    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        this.address = address;
    }

    //
    public void setMacAddr(String macAddr) {
        this.macAddr = macAddr;
    }

    public String getMacAddr() {
        return macAddr;
    }

    //
    public String getSessionKey() {
        return sessionKey;
    }

    public void setSessionKey(String sessionKey) {
        this.sessionKey = sessionKey;
    }

    //
    public void setLoginserverid(String loginserverid) {
        this.loginserverid = loginserverid;
    }

    public String getLoginserverid() {
        return loginserverid;
    }

    private long id;
    private String name;
    private String macAddr;
    private String address;
    private String sessionKey;
    private String loginserverid;
    private Date time;
}
