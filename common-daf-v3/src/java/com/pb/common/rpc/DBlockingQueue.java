package com.pb.common.rpc;

import java.net.MalformedURLException;
import java.util.Vector;
import java.util.NoSuchElementException;
import java.io.IOException;

/**
 * Client for a distributed blocking queue data structure.
 *
 * @author   Tim Heier
 * @version  1.0 4/15/2006
 */
public class DBlockingQueue {

    public static String RPC_NAME      = "DBlockingQueue";
    public static String METHOD_PUT    = "DBlockingQueue.put";
    public static String METHOD_TAKE   = "DBlockingQueue.take";
    public static String METHOD_POLL   = "DBlockingQueue.poll";
    public static String METHOD_SIZE   = "DBlockingQueue.size";
    public static String METHOD_CLEAR  = "DBlockingQueue.clear";
    public static String METHOD_QLIST  = "DBlockingQueue.queueList";

    private String nameOfQueue;

    private RpcClient rpcClient;

    private DBlockingQueue() {

    }

    public DBlockingQueue(String nameOfQueue) throws RuntimeException {
        this.nameOfQueue = nameOfQueue;

        //Local setup only - no remote calls made
        try {
            rpcClient = new RpcClient(RPC_NAME);
        }
        catch (MalformedURLException e) {
            new RuntimeException("DBlockingQueue object not registered",e);
        }
    }

    public void put(Object value) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfQueue);
        params.add(value);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_PUT, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }
    }

    public Object take() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfQueue);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_TAKE, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }
        return obj;
    }

    public Object poll(int millis) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfQueue);
        params.add(millis);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_POLL, params);
        }
        catch (RpcException e) {
            if (e.getCause() instanceof NoSuchElementException) {
                return null;
            }
            else {
                throw e;
            }
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }
        return obj;
    }

    public int size() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfQueue);

        Integer integer = null;
        try {
            integer = (Integer) rpcClient.execute(METHOD_SIZE, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }
        return integer;
    }

    public void clear() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfQueue);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_CLEAR, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }
    }

    /**
     * Special method which returns a list of queues on the DBlockingQueue server
     *
     * @return list of BlockingQueues on server
     * @throws RpcException
     */
    public static String[] queueList() throws RpcException {
        Vector params = new Vector();
        params.add("DBlockingQueue");

        RpcClient rpcClient = null;
        try {
            rpcClient = new RpcClient(RPC_NAME);
        }
        catch (MalformedURLException e) {
            new RuntimeException("DBlockingQueue object not registered",e);
        }

        String[] strArray = null;
        try {
            strArray = (String[]) rpcClient.execute(METHOD_QLIST, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }

        return strArray;
    }
}
