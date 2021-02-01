package model;

import java.util.List;

/**
 * Created by wqs on 8/1/16.
 */
public class User {
    public UserInfo getUserinfo() {
        return userinfo;
    }

    public void setUserinfo(UserInfo userinfo) {
        this.userinfo = userinfo;
    }

    public List<MyCluster> getClusters() {
        return clusters;
    }

    public void setClusters(List<MyCluster> clusters) {
        this.clusters = clusters;
    }

    public List<MyDevice> getDevices() {
        return devices;
    }

    public void setDevices(List<MyDevice> devices) {
        this.devices = devices;
    }

    public List<MyScene> getScenes() {
        return scenes;
    }

    public void setScenes(List<MyScene> scenes) {
        this.scenes = scenes;
    }

    private UserInfo userinfo;
    private List<MyCluster> clusters;
    private List<MyDevice> devices;
    private List<MyScene> scenes;
}
