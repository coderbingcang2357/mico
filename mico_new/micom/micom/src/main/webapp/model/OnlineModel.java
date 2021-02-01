package model;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.HashOperations;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Map;

/**
 * Created by wqs on 7/21/16.
 *
 * @ContextConfiguration(classes= RedisConfig.class)
 */

@Component
public class OnlineModel {

    @Autowired
    private StringRedisTemplate readonlineinfo;

    ObjectMapper jsonpaser = new ObjectMapper();

    public OnlineModel() {
    }

    public List<Client> getClient() {
        //Jedis jds = new Jedis("10.1.240.39", 6379);
        //jds.connect();
        //Map<String, String> online = jds.hgetAll("online_info");

        List<Client> ret = new ArrayList<Client>();
        HashOperations<String, String, String> ho = readonlineinfo.opsForHash();
        Map<String, String> online = ho.entries("online_info");

        for (Map.Entry<String, String> et : online.entrySet()) {
            // ret.add(new Client(Long.parseLong(et.getKey()), et.getValue(), new Date()));
            ret.add(readClient(et.getValue()));
        }
        return ret;
    }

    private Client readClient(String jsonstr) {
        Client c = new Client();

        ObjectMapper jsmap = new ObjectMapper();
        try {
            ObjectNode node = (ObjectNode) jsmap.readTree(jsonstr);
            if (node.has("devid")) {
                c.setId(node.get("devid").asLong());
            }
            if (node.has("userid")) {
                c.setId(node.get("userid").asLong());
            }
            if (node.has("address")) {
                c.setAddress(node.get("address").asText());
            }
            if (node.has("sockaddr")) {
                c.setAddress(node.get("sockaddr").asText());
            }
            if (node.has("sessionKey")) {
                c.setSessionKey(node.get("sessionKey").asText());
            }
            if (node.has("macAddr")) {
                c.setMacAddr(node.get("macAddr").asText());
            }
            if (node.has("name")) {
                c.setName(node.get("name").asText());
            }
            if (node.has("account")) {
                c.setName(node.get("account").asText());
            }
            if (node.has("loginserverid")) {
                c.setLoginserverid(node.get("loginserverid").asText());
            }
            if (node.has("lastHeartBeatTime")) {
                c.setTime(new Date(node.get("lastHeartBeatTime").asLong() * 1000));
            }
            if (node.has("lastHeartBeatTime_bin")) {
                c.setTime(new Date(node.get("lastHeartBeatTime_bin").asLong()*1000));
            }
        }
        catch (Exception e) {
            c.setName(e.toString());
        }

        return c;

        //try {
        //    Map<String, String> jsonmap = jsonpaser.readValue(jsonstr, Map.class);
        //    for (Map.Entry<String, String> et : jsonmap.entrySet()) {
        //        String key = et.getKey();
        //        String value = et.getValue();
        //        if (key == "devid" || key == "userid") {
        //            c.setId(0);//Long.parseLong(value));
        //        } else if (key == "address" || key == "sockaddr") {
        //            c.setAddress(value);
        //        } else if (key == "sessionKey") {
        //            c.setSessionKey(value);
        //        } else if (key == "macAddr") {
        //            c.setMacAddr(value);
        //        } else if (key == "name" || key == "account") {
        //            c.setName(value);
        //        } else if (key == "loginserverid") {
        //            //c.setLoginserverid(value);
        //        }
        //    }
        //} catch (Exception e) {
        //    c.setName(e.toString());
        //}
        //return c;
    }
}
