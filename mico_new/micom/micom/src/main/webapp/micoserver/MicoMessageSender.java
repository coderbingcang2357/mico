package micoserver;
import micoserver.message.MicoInternalMessage;

import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.redis.core.StringRedisTemplate;
import org.springframework.stereotype.Component;

import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Created by wqs on 8/29/17.
 */
@Component
public class MicoMessageSender {
    @Autowired
    private StringRedisTemplate m_redis;

    private static class ServerInfo{
        ServerInfo(String ip_, short port_) {
            this.ip = ip_;
            this.port = port_;
        }
        public String ip;
        public short port;
    }
    private final ServerInfo[] serverip = {
            new ServerInfo("10.1.240.39", (short)8885),
            new ServerInfo("10.1.240.44", (short)8885),
            new ServerInfo("10.232.47.24", (short)7999)
        };

    private final Logger log = LogManager.getLogger("micom");

    public void sendMessage(String msg)
    {
        m_redis.convertAndSend("pushmsg", msg);
    }

    public void sendMessage(MicoInternalMessage msgtosend){
        // tcp connect to server
        byte[] msgdata = msgtosend.toByteArray();
        ByteBuffer databuf = ByteBuffer.allocate(msgdata.length + 4);
        int len = msgdata.length + 4;
        databuf.order(ByteOrder.LITTLE_ENDIAN);
        databuf.putInt(len);
        databuf.put(msgdata);
        for (ServerInfo si : serverip) {
            sendDataToServer(si.ip, si.port, databuf.array());
        }
    }

    private void sendDataToServer(String ip, short port, byte[] data) {
        log.debug("sendto" + ip + ":" + String.valueOf(port));
        try (Socket s = new Socket())
        {
            s.connect(new InetSocketAddress(ip, port), 2 * 1000); // 2 s
            s.getOutputStream().write(data);
            s.close();
            log.debug("send ok");
        }
        //catch (IOException e)
        //{
        //    System.err.println("Don't know about host : " + e);
        //}
        catch (Exception e)
        {
            log.debug("send notify to server failed: "
                    + ip + ":"+ String.valueOf(port)
                    +" "+ e.toString());
        }
    }
}
