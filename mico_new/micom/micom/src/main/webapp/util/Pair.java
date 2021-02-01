package util;

/**
 * Created by wqs on 7/27/16.
 */
public class Pair<T, P> {
    public T first;
    public P second;

    public Pair(T t, P p) {
        this.first = t;
        this.second = p;
    }

    public T getFirst() {
        return first;
    }

    public P getSecond() {
        return second;
    }
}
