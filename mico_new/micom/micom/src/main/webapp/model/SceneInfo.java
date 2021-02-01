package model;

import java.util.Date;
import java.util.List;

/**
 * Created by wqs on 7/27/16.
 */
public class SceneInfo {
    private long ID;
    private String name;
    private long creatorID;
    private long ownerID;
    private Date createDate;
    private String description;
    private String version;
    private int scenesize;
    private String filelist;
    private int fileserverid;
    private List<SceneSharedUser> sharedUsers;
    private List<SceneConnection> connections;

    public long getID() {
        return ID;
    }

    public void setID(long ID) {
        this.ID = ID;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public long getCreatorID() {
        return creatorID;
    }

    public void setCreatorID(long creatorID) {
        this.creatorID = creatorID;
    }

    public long getOwnerID() {
        return ownerID;
    }

    public void setOwnerID(long ownerID) {
        this.ownerID = ownerID;
    }

    public Date getCreateDate() {
        return createDate;
    }

    public void setCreateDate(Date createDate) {
        this.createDate = createDate;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String description) {
        this.description = description;
    }

    public String getVersion() {
        return version;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    public int getScenesize() {
        return scenesize;
    }

    public void setScenesize(int scenesize) {
        this.scenesize = scenesize;
    }

    public String getFilelist() {
        return filelist;
    }

    public void setFilelist(String filelist) {
        this.filelist = filelist;
    }

    public int getFileserverid() {
        return fileserverid;
    }

    public void setFileserverid(int fileserverid) {
        this.fileserverid = fileserverid;
    }

    public List<SceneSharedUser> getSharedUsers() {
        return sharedUsers;
    }

    public void setSharedUsers(List<SceneSharedUser> sharedUsers) {
        this.sharedUsers = sharedUsers;
    }

    public List<SceneConnection> getConnections() {
        return connections;
    }

    public void setConnections(List<SceneConnection> connections) {
        this.connections = connections;
    }
}
