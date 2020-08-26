package com.pb.common.rpc.tests;

import com.pb.common.rpc.RpcClient;
import com.pb.common.rpc.RpcException;
import com.pb.common.rpc.DafNode;

import java.util.Vector;
import java.util.Hashtable;
import java.net.MalformedURLException;
import java.io.IOException;

import org.apache.log4j.Logger;

/**
 * @author Tim.Heier
 * @version Sep 10, 2005
 */
public class DafNodeTest {

    protected static Logger logger = Logger.getLogger(DafNodeTest.class);

    /**
     * Synchronous call
     */
    public void syncCall(RpcClient client) {

        Object result = null;

        Vector params = new Vector();
        params.addElement( new Double("-100.345") );

        logger.info("----- synchronous sample.calculate -----");

        try {
            result = client.execute("sample.calculate", params);
            logger.info("got answer back: " + result.toString() );
            return;
        }
        catch (RpcException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        logger.info("got error back");
        logger.info("");
    }

    /**
     * Asynchronous call
     */
    public void asyncCall(RpcClient client) {

        Vector params = new Vector();
        params.addElement( new Double("-200.345") );

        logger.info("----- asynchronous calculator.doSomething -----");
        client.executeAsync("calculator.doSomething", params, new CallBackHandler());
        logger.info("should see this right away!");
        logger.info("");

    }

    /**
     * Multicall, package 2 or more calls into one request
     *
     * Each call is represented by a hashtable containing:
     *
     *  1. methodName
     *  2. param list
     *
     * The calls are sent over in a vector like normal
     */
    public void multiCall(RpcClient client) {

        Object result = null;

        logger.info("----- synchronous multicall sample.calculate, calculator.doSomething -----");

        Vector params1 = new Vector();
        params1.addElement( new Double("-100.345") );

        Vector params2 = new Vector();
        params2.addElement( new Double("-200.345") );

        Hashtable call_1 = new Hashtable();
        call_1.put("methodName", "sample.calculate");
        call_1.put("params", params1);

        Hashtable call_2 = new Hashtable();
        call_2.put("methodName", "calculator.doSomething");
        call_2.put("params", params2);

        Vector calls = new Vector();
        calls.addElement( call_1 );
        calls.addElement( call_2 );

        try {
            result = client.execute("system.multicall", calls);
            logger.info("got answer back: " + result.toString() );
            return;
        }
        catch (RpcException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        logger.info("got error back");
        logger.info("");

    }

    /**
     * Synchronous call
     */
    public void throwRuntimeException(RpcClient client) {

        Object result = null;

        Vector params = new Vector();
        params.addElement( new Double("-100.345") );

        logger.info("----- sample.throwRuntimeException -----");

        try {
            result = client.execute("sample.throwRuntimeException", params);
            logger.info("should not see this!");
            return;
        }
        catch (RpcException e) {  //remote exception
            logger.info("in exception block as expected");
            e.printStackTrace();
        }
        catch (IOException e) {   //general exception
            e.printStackTrace();
        }
        logger.info("");
    }

    /**
     * Test returning an int[]
     */
    public void returnIntArray(RpcClient client) {

        int[] result = null;

        Vector params = new Vector();
        params.addElement( new Double("-100.345") );

        logger.info("----- synchronous sample.returnIntArray -----");

        try {
            result = (int[]) client.execute("sample.returnIntArray", params);
            for (int i=0; i < result.length; i++) {
                logger.info("i["+i+"]="+result[i]);
            }
            return;
        }
        catch (RpcException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        logger.info("got error back");
        logger.info("");
    }

    /**
     * Test sending an int[]
     */
    public void sendIntArray(RpcClient client) {

        Object result = null;

        int[] intArray = { 0, 1, 2 };

        Vector params = new Vector();
        params.addElement( intArray );

        logger.info("----- synchronous sample.sendIntArray -----");

        try {
            result = client.execute("sample.sendIntArray", params);
            logger.info("got answer back: " + result.toString() );
            return;
        }
        catch (RpcException e) {
            e.printStackTrace();
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        logger.info("got error back");
        logger.info("");
    }

    public static void main (String[] args) throws Exception {

        //Need a config file to initialize a Daf node
        if (args.length < 1) {
            logger.info("usage: java " + DafNodeTest.class.getName() + " <config-file> [connect url]");
            return;
        }
        String configFileName = args[0];

        DafNode.getInstance().initClient(configFileName);

        //If url was not supplied on command-line then use an RPC handler name
        String url = "sample";
        if (args.length >= 2) {
            url = args[1];
        }

        //Create an RpcClient to pass into test methods
        RpcClient client = null;
        try {
            client = new RpcClient(url);
        }
        catch (MalformedURLException e) {
            e.printStackTrace();
        }

        //Run tests
        DafNodeTest test = new DafNodeTest();
        test.syncCall(client);
        test.asyncCall(client);
        test.multiCall(client);
        test.throwRuntimeException(client);
        test.returnIntArray(client);
        test.sendIntArray(client);
        System.exit(0);
    }
}
