package com.pb.common.rpc;

/**
 *
 */
public class HandlerSpec {

    public String name;
    public String className;
    public String node;
    public String url;

    public String toString() {
        return "["+name+", "+className+", "+node+", "+url+"]";
    }

}
