package org.apache.xmlrpc;

import com.pb.common.rpc.AsyncCallback;

public class CallData {
    XmlRpcClientRequest request;
    com.pb.common.rpc.AsyncCallback callback;

    /**
     * Make a call to be queued and then executed by the next free async
     * thread
     */
    public CallData(XmlRpcClientRequest request, AsyncCallback callback) {
        this.request = request;
        this.callback = callback;
    }
}
