package com.pb.common.rpc.tests;

import com.pb.common.rpc.RpcClient;
import com.pb.common.rpc.AsyncCallback;

import java.net.URL;
import java.net.MalformedURLException;
import java.util.Vector;

import org.apache.log4j.Logger;

public class AsyncBenchmarkTest implements Runnable
{
    protected static Logger logger = Logger.getLogger(AsyncBenchmarkTest.class);

    static String url;
    static int clients = 5;
    static int loops = 100;

    int calls = 0;

    int gCalls = 0, gErrors = 0;
    long start;


    public AsyncBenchmarkTest () throws Exception
    {
        // Some JITs (Symantec, IBM) have problems with several Threads
        // starting all at the same time.
        // This initial XML-RPC call seems to pacify them.

//        RpcClient client = new RpcClient (url);
//        Vector args = new Vector ();
//        args.addElement (new Integer (123));
//        Integer integer = (Integer) client.execute ("math.abs", args);
//        System.out.println("got back: " + integer);

        start = System.currentTimeMillis ();

        for (int i = 0; i < clients; i++)
            new Thread (this).start ();
    }

    public void run ()
    {
        RpcClient client = null;
        try {
            client = new RpcClient (url);
        } catch (MalformedURLException e) {
            e.printStackTrace();
        }

        int calls = 0;
        long start = System.currentTimeMillis ();

        for (int i = 0; i < loops; i++)
        {
            Vector args = new Vector ();
            Integer n = new Integer (Math.round ((int)(Math.random () * -1000)));
            args.addElement (n);

            logger.info(Thread.currentThread().getName() + " picked: " + n);

            client.executeAsync ("math.abs", args, new Callback(n));
            calls += 1;
        }
        int millis = (int)(System.currentTimeMillis () - start);
        System.err.println ("AsyncBenchmarkTest " + Thread.currentThread().getName() + " finished: "+calls + " calls in "+
                            millis + " milliseconds.");
    }

    public static void main (String args[]) throws Exception
    {
        if (args.length > 0 && args.length < 3)
        {
            url = args[0];
//            if (args.length == 2)
//                XmlRpc.setDriver (args[1]);
            new AsyncBenchmarkTest ();
        }
        else
        {
            System.err.println ("Usage: java com.pb.common.rpc.AsyncBenchmarkTest  <URL> [SAXDriver]");
        }
    }

    class Callback implements AsyncCallback
    {
        int n;

        public Callback (Integer n)
        {
            this.n = Math.abs (n.intValue());
        }

        public synchronized void handleResult (Object result, URL url, String method)
        {
            logger.info("got back: " + result);

            if (n == ((Integer) result).intValue ())
                gCalls += 1;
            else
                gErrors += 1;
            if (gCalls + gErrors >= clients * loops)
                printStats ();
        }

        public synchronized void handleError (Exception exception, URL url, String method)
        {
            logger.info ("exception returned to handleError");
            exception.printStackTrace ();
            gErrors += 1;
            if (gCalls + gErrors >= clients * loops)
                printStats ();
        }

        public void printStats ()
        {
            System.err.println ("");
            System.err.println (gCalls + " calls, "+gErrors + " errors in "+
                    (System.currentTimeMillis() - start) + " millis");
            System.err.println ( (1000 * (gCalls + gErrors) /
                    (System.currentTimeMillis() - start)) + " calls per second");
        }

    }


}
