package model;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import util.CustomDateSerializer;

import java.util.Date;

/**
 * Created by wqs on 8/25/17.
 */
public class PushMessge {
    private long id;
    private String message;
    private Date createdate;
    private int shouldsend;

    public int getShouldsend() {
        return shouldsend;
    }

    public void setShouldsend(int shouldsend) {
        this.shouldsend = shouldsend;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getMessage() {
        return message;
    }

    public void setMessage(String message) {
        this.message = message;
    }

    @JsonSerialize(using = CustomDateSerializer.class)
    public Date getCreatedate() {
        return createdate;
    }

    public void setCreatedate(Date createdate) {
        this.createdate = createdate;
    }
}
