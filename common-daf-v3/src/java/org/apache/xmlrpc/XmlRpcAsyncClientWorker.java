package org.apache.xmlrpc;

import org.apache.xmlrpc.XmlRpcClientWorker;

import java.net.URL;
import java.util.concurrent.BlockingQueue;

import com.pb.common.rpc.RPC;

/**
 * @author Tim.Heier
 * @version Sep 17, 2005
 */
public class XmlRpcAsyncClientWorker extends XmlRpcClientWorker implements Runnable {

    BlockingQueue requestQueue;

    public XmlRpcAsyncClientWorker(URL url, BlockingQueue requestQueue)
    {
        super(url);
        this.requestQueue = requestQueue;
    }

    public XmlRpcAsyncClientWorker(String hostname, int port, BlockingQueue requestQueue)
    {
        super(hostname, port);
        this.requestQueue = requestQueue;
    }

    public void run() {

        while (true)
        {
            CallData callData = null;
            com.pb.common.rpc.AsyncCallback callback = null;
            String methodName = null;
            Object response = null;
            try {
                callData = (CallData) requestQueue.take();
                if (RPC.isDebug()) {
                    logger.debug("XmlRpcAsyncClientWorker.run["+instanceId+"] - took callData out of requestQueue");
                }

                callback = callData.callback;
                methodName = callData.request.getMethodName();

                response = execute(callData.request);

                // notify callback object
                if (callback != null)
                {
                    callback.handleResult(response, url, methodName);
                }
            }
            catch (Exception e) {
                if (callback != null) {
                    try {
                        callback.handleError(e, url, methodName);
                    }
                    catch(Exception ignore) {
                        //ignore
                    }
                }
            }
        }

    }
}
