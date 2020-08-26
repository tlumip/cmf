package com.pb.common.rpc.tests;

import com.pb.common.rpc.DHashMap;
import com.pb.common.rpc.DafNode;
import com.pb.common.rpc.RpcException;
import org.apache.log4j.Logger;

/**
 * Test the distributed HashMap data structure
 *
 * @author   Tim Heier
 * @version  4/22/06
 */
public class DHashMapTest {

    protected static Logger logger = Logger.getLogger(DHashMapTest.class);

    public static void main (String[] args) throws Exception {

        //Need a config file to initialize a Daf node
        if (args.length < 1) {
            logger.info("usage: java " + DHashMapTest.class.getName() + " <config-file>");
            return;
        }
        String configFileName = args[0];
        DafNode.getInstance().initClient(configFileName);

        putEntriesInMap("TestMap");
        getEntriesFromMap("TestMap");
        removeEntryFromMap("TestMap");
        clearMap("TestMap");

        //Print names of HashMaps created 
        String[] strArray = DHashMap.mapList();
        for (String str : strArray) {
            logger.info("Map name=" + str);
        }
    }

    public static void putEntriesInMap(String mapName) throws RpcException {
        logger.info("putEntriesInMap=" + mapName);

        DHashMap testMap = new DHashMap(mapName);
        testMap.put("entry1", "value1");
        testMap.put("entry2", "value2");

        int size = testMap.size();
        logger.info("entries in map=" + size);

        boolean b = testMap.containsKey("entry1");
        logger.info("entry1 in map=" + b);

        String[] keys = testMap.keyList();
        String keyList = "";
        for (String key : keys) {
            keyList += key + "  "; 
        }

        logger.info("list of keys= [ " + keyList + " ]");

    }

    public static void getEntriesFromMap(String mapName) throws RpcException {
        logger.info("getEntriesFromMap=" + mapName);

        //Access map and print size
        DHashMap testMap = new DHashMap(mapName);
        int size = testMap.size();
        logger.info("entries in map=" + size);

        //Remove one element from map
        String value = (String) testMap.get("entry1");
        logger.info("value="+value);

        //Print size again
        size = testMap.size();
        logger.info("entries in map=" + size);
    }

    public static void removeEntryFromMap(String mapName) throws RpcException {
        logger.info("removeEntryFromMap=" + mapName);

        //Access map and print size
        DHashMap testMap = new DHashMap(mapName);
        int size = testMap.size();
        logger.info("entries in map=" + size);

        //Remove one element from map
        boolean b = testMap.remove("entry1");
        logger.info("removed value="+b);

        testMap.containsKey("entry1");
        logger.info("entry1 in map=" + b);

        //Print size again
        size = testMap.size();
        logger.info("entries in map=" + size);
    }

    public static void clearMap(String mapName) throws RpcException {
        logger.info("clearMap=" + mapName);

        //Access map and print size
        DHashMap testMap = new DHashMap(mapName);
        int size = testMap.size();
        logger.info("entries in map before clear=" + size);

        testMap.clear();

        //Print size again
        size = testMap.size();
        logger.info("entries in map after clear=" + size);
    }
}
