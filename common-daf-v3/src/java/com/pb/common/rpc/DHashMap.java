package com.pb.common.rpc;

import java.net.MalformedURLException;
import java.util.Vector;
import java.util.HashMap;
import java.util.NoSuchElementException;
import java.io.IOException;

/**
 * Client for a distributed HashMap data structure.
 *
 * @author   Tim Heier
 * @version  1.0 4/15/2006
 */
public class DHashMap {

    public static String RPC_NAME       = "DHashMap";
    public static String METHOD_PUT     = "DHashMap.put";
    public static String METHOD_GET     = "DHashMap.get";
    public static String METHOD_SIZE    = "DHashMap.size";
    public static String METHOD_REMOVE  = "DHashMap.remove";
    public static String METHOD_CLEAR   = "DHashMap.clear";
    public static String METHOD_KEY     = "DHashMap.containsKey";
    public static String METHOD_KEYLIST = "DHashMap.keyList";
    public static String METHOD_MAPLIST = "DHashMap.mapList";
    public static String METHOD_GETMAP  = "DHashMap.getMap";

    private String nameOfMap;

    private RpcClient rpcClient;

    private DHashMap() {

    }

    public DHashMap(String nameOfMap) throws RuntimeException {
        this.nameOfMap = nameOfMap;

        //Local setup only - no remote calls made
        try {
            rpcClient = new RpcClient(RPC_NAME);
        }
        catch (MalformedURLException e) {
            new RuntimeException("DHashMap object not registered",e);
        }
    }

    public Object put(String key, Object value) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);
        params.add(key);
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

        return obj;
    }

    public Object get(String key) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);
        params.add(key);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_GET, params);
        }
        catch (RpcException e) {
            //No value for key - return null
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

    public boolean remove(String key) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);
        params.add(key);

        Boolean bool = null;
        try {
            bool = (Boolean) rpcClient.execute(METHOD_REMOVE, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }

        return bool;
    }

    public Object clear() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);

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

        return obj;
    }

    public int size() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);

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

    public String[] keyList() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);

        String[] strArray = null;
        try {
            strArray = (String[]) rpcClient.execute(METHOD_KEYLIST, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }

        return strArray;
    }

    public boolean containsKey(String key) throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);
        params.add(key);

        Boolean bool = null;
        try {
            bool = (Boolean) rpcClient.execute(METHOD_KEY, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }

        return bool;
    }

    public HashMap getMap() throws RpcException {
        Vector params = new Vector();
        params.add(this.nameOfMap);

        Object obj = null;
        try {
            obj = rpcClient.execute(METHOD_GETMAP, params);
        }
        catch (RpcException e) {
            throw e;
        }
        catch (IOException e) {
            throw new RpcException("IOException", e);
        }

        return (HashMap)obj;
    }

    /**
     * Special method which returns a list of maps on the DHashMap server
     *
     * @return list of HashMaps on server
     * @throws RpcException
     */
    public static String[] mapList() throws RpcException {
        Vector params = new Vector();
        params.add("DHashMap");

        RpcClient rpcClient = null;
        try {
            rpcClient = new RpcClient(RPC_NAME);
        }
        catch (MalformedURLException e) {
            new RuntimeException("DHashMap object not registered",e);
        }

        String[] strArray = null;
        try {
            strArray = (String[]) rpcClient.execute(METHOD_MAPLIST, params);
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
