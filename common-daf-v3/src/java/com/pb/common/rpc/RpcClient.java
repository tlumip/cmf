package com.pb.common.rpc;

import org.apache.log4j.Logger;
import org.apache.xmlrpc.*;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Vector;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.io.IOException;

/**
 * A multithreaded, reusable XML-RPC client object. This version can use a homegrown
 * TCP client which can be quite a bit faster than HTTP, especially when used with
 * XmlRpc.setKeepAlive(true).
 *
 * @author   Tim Heier
 * @version
 */
public class RpcClient {

    protected static Logger logger = Logger.getLogger(RpcClient.class);

    protected static int idCounter = 0;

    protected int instanceId = 0;

    protected String protocol;
    protected String hostname;
    protected int port = 80;

    protected URL url;

    //used to make synchronous calls
    protected XmlRpcClientWorker syncWorker;

    //used to store asynchronous calls
    protected BlockingQueue requestQueue;

    /**
     * Construct a XML-RPC client for the URL or handlerName String passed in.
     */
    public RpcClient (String clientString) throws MalformedURLException
    {
        synchronized (RpcClient.class) {
            instanceId = idCounter++;
        }

        String urlString = null;

        // if handler is null, i.e. clientString is just a url (protocol://hostname:port) 
        // extract the protocol, hostname and port from the clientString;
        // otherwise, clientString is the name of a handler defined in a config file, so
        // get the url to which the handler is associated, and extract the protocol, hostname and port.
        
        HandlerSpec handler = (HandlerSpec) DafNode.getInstance().handlerToNodeMap.get(clientString);
        
        if (handler == null) {
            urlString = clientString;
        }
        else {
            urlString = handler.url;
        }

        extractHostAndPort(urlString);

        if (logger.isDebugEnabled()) {
            logger.debug("RpcClient.init["+instanceId+"] - clientString=" + clientString + 
                    ", urlString=" + urlString + ", protocol=" + protocol + 
                    ", hostname=" + hostname + ", port=" + port);
        }
    }

    private void extractHostAndPort(String urlAsString) throws MalformedURLException
    {
        //Split url into protocol, hostname and port
        if (urlAsString.startsWith("http")) {
            protocol = "http";
            url = new URL(urlAsString);  //used by http client worker
        }
        else if (urlAsString.startsWith("tcp")) {
            protocol = "tcp";
            url = null;
        }

        //extract hostname and port used by tcp client worker
        int begin = urlAsString.indexOf("//");
        if (begin < 0) {
            throw new MalformedURLException("could not determine hostname from: " + url);
        }
        begin += 2;

        //extract port
        int end = urlAsString.lastIndexOf(':');

        if (end < 0 || end < begin) {  //no port
            hostname = urlAsString.substring(begin);
            port = 80;  //default
        }
        else {
            hostname = urlAsString.substring(begin, end);
            port = Integer.parseInt(urlAsString.substring(end+1));
        }
    }

    synchronized public Object execute(String method, Vector params) throws RpcException, IOException
    {
        if (logger.isDebugEnabled()) {
            logger.debug("RpcClient.execute["+instanceId+"] - "+method);
        }

        return execute(new XmlRpcRequest(method, params));
    }

    private Object execute(XmlRpcClientRequest request) throws RpcException, IOException
    {
        //delay creating syncWorker until needed
        if (syncWorker == null) {
            if (protocol.equalsIgnoreCase("tcp")) {
                syncWorker = new XmlRpcClientWorker(hostname, port);
            }
            //must be http protocol
            else {
                syncWorker = new XmlRpcClientWorker(url);
            }
        }
        Object retval = null;
        try {
            retval = syncWorker.execute(request);
        }
        catch (XmlRpcClientException e) {    //Generated on client side
            throw new RuntimeException(e);
        }

        //Result is an exception, so wrap it and throw it
        if (retval instanceof Exception)
            throw new RpcException((Exception)retval);

        return retval;
    }

    public void executeAsync(String method, Vector params, AsyncCallback callback) {

        if (RPC.isDebug()) {
            logger.debug("RpcClient.executeAsync["+instanceId+"] - " + method);
        }

        //delay creating async requestQueue and workers until needed
        if (requestQueue == null) {
            requestQueue = new LinkedBlockingQueue(1000);

            for (int i=0; i < 2; i++) {
                if (protocol.equalsIgnoreCase("tcp")) {
                    new Thread( new XmlRpcAsyncClientWorker(hostname, port, requestQueue), "AsyncWorker_"+i ).start();
                }
                else {
                    new Thread( new XmlRpcAsyncClientWorker(url, requestQueue), "AsyncWorker_"+i ).start();
                }
            }
            try {
                Thread.sleep(250);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        try {
            CallData callData = new CallData(new XmlRpcRequest(method, params), callback);
            requestQueue.put(callData);
        }
        catch (Exception e) {
            logger.error("exception executing  METHOD=" + method + "  URL=" + this.url, e);
        }
    }

    /**
     * Just for testing.
     */
    public static void main(String args[]) throws Exception
    {
        //XmlRpc.setDebug (true);
        try
        {
            String url = args[0];
            String method = args[1];
            RpcClient client = new RpcClient (url);
            Vector v = new Vector ();
            for (int i = 2; i < args.length; i++) {
                try {
                    v.addElement(new Integer(Integer.parseInt(args[i])));
                }
                catch (NumberFormatException nfx) {
                    v.addElement(args[i]);
                }
            }
            try {
                System.out.println(client.execute(method, v));
            }
            catch (Exception ex) {
                System.err.println("Error: " + ex.getMessage());
            }
            finally {
                System.out.println("done");
            }
        }
        catch (Exception x) {
            System.err.println(x);
            System.err.println("Usage:  java  RpcClient " + "<url>  <method>  <arg1>  <arg2>...");
            System.err.println("Arguments are sent as integers or strings.");
        }
    }
}
