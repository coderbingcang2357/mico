package micoserver.message;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Arrays;

/**
 * Created by wqs on 8/29/17.
 */
public class MicoInternalMessage {
    private Message message;
    private short version;
    private short type;
    private short subtype;
    private byte encryptMethod;

    public MicoInternalMessage() {
        version = 0;
        type = 0;
        subtype = 0;
        encryptMethod = 0;
    }

    public byte[] toByteArray() {
        ByteBuffer bb = ByteBuffer.allocate(1024);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.putShort(version);
        bb.putShort(type);
        bb.putShort(subtype);
        bb.put(encryptMethod);
        bb.put(message.toByteArray());

        return Arrays.copyOfRange(bb.array(), 0, bb.position());
    }

    public Message getMessage() {
        return message;
    }

    public void setMessage(Message message) {
        this.message = message;
    }

    public short getVersion() {
        return version;
    }

    public void setVersion(short version) {
        this.version = version;
    }

    public short getType() {
        return type;
    }

    public void setType(short type) {
        this.type = type;
    }

    public short getSubtype() {
        return subtype;
    }

    public void setSubtype(short subtype) {
        this.subtype = subtype;
    }

    public byte getEncryptMethod() {
        return encryptMethod;
    }

    public void setEncryptMethod(byte encryptMethod) {
        this.encryptMethod = encryptMethod;
    }
}
