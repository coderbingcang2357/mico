package micoserver.message;

import java.nio.*;
import java.util.Arrays;

/**
 * Created by wqs on 8/28/17.
 */
public class Message {
    public Message() {
        prefix = 0;
        serialNumber = 0;
        commandID = 0;
        versionNumber = 0;
        objectID = 0;
        destID = 0;
        suffix = 0;
        content = null;
    }

    byte[] toByteArray() {
        ByteBuffer bu = ByteBuffer.allocate(1024);
        bu.order(ByteOrder.LITTLE_ENDIAN);
        bu.put(prefix);
        bu.putShort(serialNumber);
        bu.putShort(commandID);
        bu.putShort(versionNumber);
        bu.putLong(objectID);
        bu.putLong(destID);
        short contentlen = (short) content.length;
        bu.putShort(contentlen);
        bu.put(content);
        bu.put(suffix);

        return Arrays.copyOfRange(bu.array(), 0, bu.position());
    }

    public byte getPrefix() {
        return prefix;
    }

    public void setPrefix(byte prefix) {
        this.prefix = prefix;
    }

    public short getSerialNumber() {
        return serialNumber;
    }

    public void setSerialNumber(short serialNumber) {
        this.serialNumber = serialNumber;
    }

    public short getCommandID() {
        return commandID;
    }

    public void setCommandID(short commandID) {
        this.commandID = commandID;
    }

    public short getVersionNumber() {
        return versionNumber;
    }

    public void setVersionNumber(short versionNumber) {
        this.versionNumber = versionNumber;
    }

    public long getObjectID() {
        return objectID;
    }

    public void setObjectID(long objectID) {
        this.objectID = objectID;
    }

    public long getDestID() {
        return destID;
    }

    public void setDestID(long destID) {
        this.destID = destID;
    }

    public byte getSuffix() {
        return suffix;
    }

    public void setSuffix(byte suffix) {
        this.suffix = suffix;
    }

    public byte[] getContent() {
        return content;
    }

    public void setContent(byte[] content) {
        this.content = content;
    }

    private byte prefix;
    private short serialNumber;
    private short commandID;
    private short versionNumber;
    private long objectID;
    private long destID;
    private byte suffix;
    private byte[] content;
}
