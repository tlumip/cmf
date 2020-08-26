package com.pb.common.rpc;

import org.apache.log4j.Logger;

import java.util.*;

/**
 * Server implementation of distributed HashMap data structure.
 *
 * @author   Tim Heier
 * @version  1.0 4/15/2006
 */
public class DHashMapHandler implements RpcHandler {

    protected static Logger logger = Logger.getLogger(DHashMapHandler.class);

    HashMap mapOfMaps = new HashMap();

    public Object execute(String methodName, Vector params) throws Exception {
        if (logger.isDebugEnabled()) {
            logger.debug("DHashMapHandler - methodName="+methodName);
            for (Object obj : params) {
                logger.debug("param="+obj);
            }
        }

        if (methodName.equalsIgnoreCase("put")) {
            String mapName = (String) params.elementAt(0);
            String key = (String) params.elementAt(1);
            Object value = (Object) params.elementAt(2);
            return put(mapName, key, value);
        }
        else
        if (methodName.equalsIgnoreCase("get")) {
            String mapName = (String) params.elementAt(0);
            String key = (String) params.elementAt(1);
            return get(mapName, key);
        }
        else
        if (methodName.equalsIgnoreCase("size")) {
            String mapName = (String) params.elementAt(0);
            return size(mapName);
        }
        else
        if (methodName.equalsIgnoreCase("remove")) {
            String mapName = (String) params.elementAt(0);
            String key = (String) params.elementAt(1);
            return remove(mapName, key);
        }
        else
        if (methodName.equalsIgnoreCase("clear")) {
            String mapName = (String) params.elementAt(0);
            return clear(mapName);
        }
        else
        if (methodName.equalsIgnoreCase("containsKey")) {
            String mapName = (String) params.elementAt(0);
            String key = (String) params.elementAt(1);
            return containsKey(mapName, key);
        }
        else
        if (methodName.equalsIgnoreCase("keyList")) {
            String mapName = (String) params.elementAt(0);
            return keyList(mapName);
        }
        else
        if (methodName.equalsIgnoreCase("mapList")) {
            String mapName = (String) params.elementAt(0);
            return mapList(mapName);
        }
        else
        if (methodName.equalsIgnoreCase("getMap")) {
            String mapName = (String) params.elementAt(0);
            return getMap(mapName);
        }
        else {
            throw new RuntimeException("DHashMapHandler.execute - methodName="+methodName + " not found");
        }
    }

    public Object put(String mapName, String key, Object value) {
        HashMap theMap = getMap(mapName);
        theMap.put(key, value);

        return new Boolean(true);
    }

    public Object get(String mapName, String key) {
        HashMap theMap = getMap(mapName);
        Object obj = theMap.get(key);

        //Cannot return null - thrown NoSuchElementException and catch in DHashMap class
        if (obj == null) {
            throw new NoSuchElementException("No entry for key="+key);
        }

        return obj;
    }

    public Object size(String mapName) {
        HashMap theMap = getMap(mapName);
        return theMap.size();
    }

    public Object containsKey(String mapName, String key) {
        HashMap theMap = getMap(mapName);
        boolean bool = theMap.containsKey(key);
        return new Boolean(bool);
    }

    public Object remove(String mapName, String key) {
        HashMap theMap = getMap(mapName);
        Object obj = theMap.remove(key);

        boolean removed = true;

        if (obj == null) {
            removed = false;
        }
        return new Boolean(removed);
    }

    public Object clear(String mapName) {
        HashMap theMap = getMap(mapName);
        theMap.clear();

        return new Boolean(true);
    }

    public Object keyList(String mapName) {
        HashMap theMap = getMap(mapName);

        //Creat a String[] to hold the key names
        Set keySet = theMap.keySet();
        String[] strArray = new String[keySet.size()];

        //Iterate over keys in Map
        int i=0;
        for (Iterator it = theMap.entrySet().iterator(); it.hasNext();) {
            Map.Entry entry = (Map.Entry) it.next();
            strArray[i++] = (String) entry.getKey();
            //Object key = entry.getKey();
            //Object value = entry.getValue();
        }
        return strArray;
    }

    public Object mapList(String mapName) {
        //Creat a String[] to hold the map names
        Set keySet = mapOfMaps.keySet();
        String[] strArray = new String[keySet.size()];

        //Iterate over keys in Map to get names of maps held
        int i=0;
        for (Iterator it = mapOfMaps.entrySet().iterator(); it.hasNext();) {
            Map.Entry entry = (Map.Entry) it.next();
            strArray[i++] = (String) entry.getKey();
            //Object key = entry.getKey();
            //Object value = entry.getValue();
        }
        return strArray;
    }

    //Returns map if it exists - otherwise an exception is thrown
    private HashMap getMap(String mapName) {
        HashMap theMap = (HashMap) mapOfMaps.get(mapName);

        if (theMap == null) {
            theMap = new HashMap(100);
            mapOfMaps.put(mapName, theMap);
        }
        return theMap;
    }

}
