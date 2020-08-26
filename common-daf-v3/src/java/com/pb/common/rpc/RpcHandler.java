package com.pb.common.rpc;

import org.apache.xmlrpc.XmlRpcHandler;

import java.util.Vector;

/**
 * @author    Tim.Heier
 * @version   1.0, Sep 19, 2005
 */
public interface RpcHandler extends XmlRpcHandler {

    /**
     * Return the result, or throw an Exception if something went wrong
     */
    public Object execute (String methodName, Vector params) throws Exception;
}
