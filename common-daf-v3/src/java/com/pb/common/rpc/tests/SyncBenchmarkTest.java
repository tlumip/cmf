package com.pb.common.rpc.tests;

import com.pb.common.rpc.RpcClient;

import java.io.IOException;
import java.util.Date;
import java.util.Vector;

import org.apache.log4j.Logger;

public class SyncBenchmarkTest implements Runnable
{
    protected static Logger logger = Logger.getLogger(SyncBenchmarkTest.class);

    RpcClient client;
    static String url;
    static int clients = 5;
    static int loops = 100;

    long start;

    int gCalls = 0, gErrors = 0;

    Date date;

    public SyncBenchmarkTest () throws Exception
    {
        client = new RpcClient (url);

        Vector args = new Vector ();
        // Some JITs (Symantec, IBM) have problems with several Threads
        // starting all at the same time.
        // This initial XML-RPC call seems to pacify them.

//        args.addElement (new Integer (-123));
//        Integer integer = (Integer) client.execute ("math.abs", args);
//        System.out.println("got back: " + integer);

        date = new Date ();
        date = new Date ((date.getTime() / 1000) * 1000);

        start = System.currentTimeMillis ();
        int nclients = clients;

        for (int i = 0; i < nclients; i++)
            new Thread (this).start ();
    }

    public void run ()
    {
        int errors = 0;
        int calls = 0;
        try
        {
            int val = (int)(-100 * Math.random ());
            logger.info(Thread.currentThread().getName() + " picked: " + val);

            Vector args = new Vector ();
            args.addElement (new Integer (val));

            for (int i = 0; i < loops; i++)
            {
                Integer ret;

                //Synchronize the calls because the client connection is being
                //shared amoung the threads
                synchronized (client)
                {
                    ret = (Integer) client.execute ("math.abs", args);
                    logger.info("got back: " + ret);
                }

                if (ret.intValue () != Math.abs (val))
                {
                    errors += 1;
                }
                calls += 1;
            }
        }
        catch (IOException x)
        {
            System.err.println ("Exception in client: "+x);
            x.printStackTrace ();
        }
        catch (Exception other)
        {
            System.err.println ("Exception in SyncBenchmarkTest client: "+other);
        }
        int millis = (int)(System.currentTimeMillis () - start);
        checkout (calls, errors, millis);
    }

    private synchronized void checkout (int calls, int errors, int millis)
    {
        clients--;
        gCalls += calls;
        gErrors += errors;
        System.err.println (Thread.currentThread().getName() + " finished: "+calls + " calls, "+
                errors + " errors in "+millis + " milliseconds.");
        if (clients == 0)
        {
            System.err.println ("");
            System.err.println ("SyncBenchmarkTest result: "+
                    (1000 * gCalls / millis) + " calls per second.");
        }
    }

    public static void main (String args[]) throws Exception
    {
        if (args.length > 0 && args.length < 3)
        {
            url = args[0];
            new SyncBenchmarkTest ();
        }
        else
        {
            System.err.println ("Usage: java com.pb.common.rpc.tests.SyncBenchmarkTest  <URL>  [SAXDriver]");
        }
    }

}
