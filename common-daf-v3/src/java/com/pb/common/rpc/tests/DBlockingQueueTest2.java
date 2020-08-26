package com.pb.common.rpc.tests;

import org.apache.log4j.Logger;
import com.pb.common.rpc.DafNode;
import com.pb.common.rpc.DBlockingQueue;
import com.pb.common.rpc.RpcException;

/**
 * Test the distributed BlockingQueue data structure
 *
 * @author   Tim Heier
 * @version  4/22/06
 */
public class DBlockingQueueTest2 {

    protected static Logger logger = Logger.getLogger(DBlockingQueueTest2.class);

    public static void main (String[] args) throws Exception {

        //Need a config file to initialize a Daf node
        if (args.length < 1) {
            logger.info("usage: java " + DBlockingQueueTest.class.getName() + " <config-file>");
            return;
        }
        String configFileName = args[0];
        DafNode.getInstance().initClient(configFileName);

        //putEntriesInQueue(3, "TestQueue");
        //takeEntriesFromQueue(3, "TestQueue");
        pollForEntryFromQueue("TestQueue");

        //Print names of HashMaps created
        String[] strArray = DBlockingQueue.queueList();
        for (String str : strArray) {
            logger.info("Queue name=" + str);
        }
    }

    public static void putEntriesInQueue(int numEntries, String queueName) throws RpcException {
        logger.info("put entries in=" + queueName);

        DBlockingQueue testQueue = new DBlockingQueue(queueName);

        //tch
        System.out.println("hello 00");
        for (int i=1; i<= numEntries; i++) {
            testQueue.put("entry_" + i+3);
        }

        int size = testQueue.size();
        logger.info(queueName + ".size=" + size);
    }

    public static void takeEntriesFromQueue(int numEntries, String queueName) throws RpcException {
        logger.info("take entries from=" + queueName);

        String value = null;

        //Access same queue by name
        DBlockingQueue testQueue = new DBlockingQueue(queueName);

        for (int i=1; i<= numEntries; i++) {
            value = (String) testQueue.take();
            logger.info("value="+value);
        }

        int size = testQueue.size();
        logger.info(queueName + ".size=" + size);
    }

    public static void pollForEntryFromQueue(String queueName) throws RpcException {
        logger.info("poll for entry from=" + queueName);

        String value = null;

        //Access same queue by name
        DBlockingQueue testQueue = new DBlockingQueue(queueName);
        value = (String) testQueue.poll(5000);

        logger.info("value=" + value);

        int size = testQueue.size();
        logger.info(queueName + ".size=" + size);
    }

    public static void clearQueue(String queueName) throws RpcException {
        logger.info(queueName + " has been cleared");
        //Access same queue by name
        DBlockingQueue testQueue = new DBlockingQueue(queueName);
        testQueue.clear();

        int size = testQueue.size();
        logger.info(queueName + ".size=" + size);
    }
}
