package model;

import org.springframework.stereotype.Component;

import java.util.ArrayList;

/**
 * Created by wqs on 2/10/17.
 */
@Component
public class ServerInfoRepos {
    public ServerInfoRepos() {
        //ServerInfo info1 = new ServerInfo();
        //info1.setId(1);
        //info1.setIp("10.1.240.39");
        //info1.setPortlogical(8885);
        //servers.add(info1);

        //ServerInfo info3 = new ServerInfo();
        //info3.setId(3);
        //info3.setIp("10.1.240.44");
        //info3.setPortlogical(8885);
        //servers.add(info3);

        ServerInfo info1 = new ServerInfo();
        info1.setId(2);
        info1.setIp("10.232.47.24");
        info1.setPortlogical(8885);
        servers.add(info1);
    }
    public ArrayList<ServerInfo> getServers() {
        return servers;
    }

    public String getServerIp(int id) {
        for (ServerInfo s : servers) {
            if (s.getId() == id)
                return s.getIp();
        }
        return "";
    }

    private ArrayList<ServerInfo> servers = new ArrayList<>();
}
