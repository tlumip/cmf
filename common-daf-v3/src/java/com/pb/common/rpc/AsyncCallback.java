package com.pb.common.rpc;

import java.net.URL;

/**
 * A callback interface for an asynchronous RPC call.
 *
 */
public interface AsyncCallback extends org.apache.xmlrpc.AsyncCallback
{
    /**
     * Call went ok, handle result.
     */
    public void handleResult(Object result, URL url, String method);

    /**
     * Something went wrong, handle error.
     */
    public void handleError(Exception exception, URL url, String method);
}
