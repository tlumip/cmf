package com.pb.common.rpc;

import org.apache.log4j.Logger;

import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

/**
 * Server implementation of distributed BlockingQueue data structure.
 *
 * @author   Tim Heier
 * @version  1.0 4/15/2006
 */
public class DBlockingQueueHandler implements RpcHandler {

    protected static Logger logger = Logger.getLogger(DBlockingQueueHandler.class);

    HashMap mapOfQueues = new HashMap();

    public Object execute(String methodName, Vector params) throws Exception {
        if (logger.isDebugEnabled()) {
            logger.debug("DBlockingQueueHandler - methodName="+methodName);
            for (Object obj : params) {
                logger.debug("param="+obj);
            }
        }

        if (methodName.equalsIgnoreCase("put")) {
            String queueName = (String) params.elementAt(0);
            Object value = (Object) params.elementAt(1);
            return put(queueName, value);
        }
        else
        if (methodName.equalsIgnoreCase("take")) {
            String queueName = (String) params.elementAt(0);
            return take(queueName);
        }
        if (methodName.equalsIgnoreCase("poll")) {
            String queueName = (String) params.elementAt(0);
            int millis = (Integer) params.elementAt(1);
            return poll(queueName, millis);
        }
        else
        if (methodName.equalsIgnoreCase("size")) {
            String queueName = (String) params.elementAt(0);
            return size(queueName);
        }
        else
        if (methodName.equalsIgnoreCase("clear")) {
            String queueName = (String) params.elementAt(0);
            return clear(queueName);
        }
        else
        if (methodName.equalsIgnoreCase("queueList")) {
            String mapName = (String) params.elementAt(0);
            return queueList(mapName);
        }
        else {
            throw new RuntimeException("DBlockingQueueHandler.execute - methodName="+methodName + " not found");
        }
    }

    public Object put(String queueName, Object value) {
        BlockingQueue theQueue = getQueue(queueName);
        try {
            theQueue.put(value);
        } catch (InterruptedException e) {
            throw new RuntimeException(("BlockingQueue.put interrupted for queue=" + queueName), e);
        }

        return new Boolean(true);
    }

    public Object take(String queueName) {
        BlockingQueue theQueue = getQueue(queueName);

        Object obj = null;
        try {
            obj = theQueue.take();
        } catch (InterruptedException e) {
            throw new RuntimeException(("BlockingQueue.take interrupted for queue=" + queueName), e);
        }

        return obj;
    }

    public Object poll(String queueName, int millis) {
        BlockingQueue theQueue = getQueue(queueName);

        Object obj = null;
        try {
            obj = theQueue.poll(millis, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            throw new RuntimeException(("BlockingQueue.take interrupted for queue=" + queueName), e);
        }

        if (obj == null) {
            throw new NoSuchElementException("Time expired while waiting for an element");
        }
        return obj;
    }

    public Object size(String queueName) {
        BlockingQueue theQueue = getQueue(queueName);
        return theQueue.size();
    }

    public Object clear(String queueName) {
        BlockingQueue theQueue = getQueue(queueName);
        theQueue.clear();
        return new Boolean(true);
    }

    public Object queueList(String mapName) {

        //Creat a String[] to hold the map names
        Set keySet = mapOfQueues.keySet();
        String[] strArray = new String[keySet.size()];

        //Iterate over keys in Map to get name of queues
        int i=0;
        for (Iterator it = mapOfQueues.entrySet().iterator(); it.hasNext();) {
            Map.Entry entry = (Map.Entry) it.next();
            strArray[i++] = (String) entry.getKey();
            //Object key = entry.getKey();
            //Object value = entry.getValue();
        }
        return strArray;
    }

    //Create a BlockingQueue of unlimited size - probably first time access
    private BlockingQueue getQueue(String queueName) {
        BlockingQueue theQueue = (BlockingQueue) mapOfQueues.get(queueName);

        //BlockingQueue does not exist yet - create it
        if (theQueue == null) {
            theQueue = new LinkedBlockingQueue();
            mapOfQueues.put(queueName, theQueue);
        }

        return theQueue;
    }

}
