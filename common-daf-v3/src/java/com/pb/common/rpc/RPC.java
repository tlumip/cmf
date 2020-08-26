package com.pb.common.rpc;

import org.apache.xmlrpc.XmlRpc;

/**
 * @author   Tim.Heier
 * @version  Sep 5, 2005
 */
public class RPC {

    protected static boolean debug = false;
    protected static boolean logPerformance = false;

    static {
        XmlRpc.setDebug(false);
        XmlRpc.setKeepAlive(true);
    }

    public static boolean isDebug() {
        return debug;
    }

    public static void setDebug(boolean debug) {
        RPC.debug = debug;
    }

    public static boolean isLogPerformance() {
        return logPerformance;
    }

    public static void setLogPerformance(boolean logPerformance) {
        RPC.logPerformance = logPerformance;
    }

    //Delegate to XmlRpc class
    public static boolean isKeepAlive() {
        return XmlRpc.getKeepAlive();
    }

    //Delegate to XmlRpc class
    public static void setKeepAlive(boolean keepAlive) {
        XmlRpc.setKeepAlive(keepAlive);
    }
}
