package model;

/**
 * Created by wqs on 2/10/17.
 */
public class ServerInfo {
    private int id;
    private String ip;
    private int portlogical;
    private int portext;

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public int getPortlogical() {
        return portlogical;
    }

    public void setPortlogical(int portlogical) {
        this.portlogical = portlogical;
    }

    public int getPortext() {
        return portext;
    }

    public void setPortext(int portext) {
        this.portext = portext;
    }

    public int getPortfile() {
        return portfile;
    }

    public void setPortfile(int portfile) {
        this.portfile = portfile;
    }

    private int portfile;
}
