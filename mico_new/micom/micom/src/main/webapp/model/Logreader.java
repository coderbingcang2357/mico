package model;
import java.net.*;
import java.nio.ByteBuffer;

import static java.nio.ByteOrder.BIG_ENDIAN;

/**
 * Created by wqs on 2/10/17.
 */
public class Logreader {
    private static final int PACKETSIZE = 64 * 1024;

    public static String readlog(String server, int size) {
        DatagramSocket socket = null ;

        String log = "";
        try
        {
            // Convert the arguments first, to ensure that they are valid
            InetAddress host = InetAddress.getByName( server ) ;
            int port         = 8883;

            // Construct the socket
            socket = new DatagramSocket() ;

            // Construct the datagram packet
            short getlogcmd = 0x2;
            short ssize = (short)size;
            byte [] data = new byte[4] ;
            ByteBuffer bb = ByteBuffer.wrap(data);
            bb.order(BIG_ENDIAN); // big endian is network order
            bb.putShort(getlogcmd);
            bb.putShort(ssize);
            DatagramPacket packet = new DatagramPacket( data, data.length, host, port ) ;

            // Send it
            socket.send( packet ) ;

            // Set a receive timeout, 2000 milliseconds
            socket.setSoTimeout( 2000 ) ;

            // Prepare the packet for receive
            byte[] recvdata = new byte[PACKETSIZE];
            packet.setData( recvdata ) ;

            // Wait for a response from the server
            socket.receive( packet ) ;

            // Print the response
            log = new String(packet.getData());

        }
        catch( Exception e )
        {
            log = e.toString();
        }
        finally
        {
            if( socket != null )
                socket.close() ;
        }
        return log;
    }
}
