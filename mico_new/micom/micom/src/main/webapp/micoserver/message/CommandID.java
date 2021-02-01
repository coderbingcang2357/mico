package micoserver.message;

/**
 * Created by wqs on 8/29/17.
 */
public class CommandID {
    public static short IN_CMD_NEW_PUSH_MESSAGE = 0x001E;
    public static byte CLIENT_MSG_PREFIX = (byte)0xF0;  //消息开始标识
    public static byte CLIENT_MSG_SUFFIX = (byte)0xF1;  //消息结束标识

    // message type
    public static short MESSAGE_TYPE_FROM_EXT_SERVER = 1;
    public static short MESSAGE_TYPE_UNUSED = 0;
}
