package com.pb.common.rpc.tests;

import org.apache.log4j.Logger;
import com.pb.common.rpc.RpcClient;
import com.pb.common.rpc.RpcException;

import java.util.Vector;
import java.util.Hashtable;
import java.io.IOException;
import java.net.MalformedURLException;

public class BootstrapNodeTest {

    protected static Logger logger = Logger.getLogger(BootstrapNodeTest.class);

    /**
     *
     */
    public void startTestClass(RpcClient client)
    {
        Object result = null;

        Hashtable env = new Hashtable();
//        env.put("classpath", "z:/myfiles/subversion/cmf/common-daf-v3/build/classes");

        Vector args = new Vector();
        args.add("java.exe");
        args.add("com.pb.common.rpc.tests.TestContent");

        Vector params = new Vector();
        params.addElement( env );
        params.addElement( args );

        try {
            result = client.execute("execute.start", params);
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
     *
     */
    public void startNode(RpcClient client)
    {
        Object result = null;

        Hashtable env = new Hashtable();
//        env.put("classpath", "z:/myfiles/subversion/cmf/common-daf-v3/build/classes");

        Vector args = new Vector();
        args.add("java.exe");
        args.add("com.pb.common.rpc.DafNode");
        args.add("-node");
        args.add("node0");
        args.add("-config");
        args.add("testapp.groovy");

        Vector params = new Vector();
        params.addElement( env );
        params.addElement( args );

        try {
            result = client.execute("execute.start", params);
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
     *
     */
    public void getStatus(RpcClient client)
    {
        Object result = null;

        Vector params = new Vector();

        try {
            result = client.execute("execute.stderr", params);
            logger.info("stderr: " + result.toString() );

            result = client.execute("execute.stdout", params);
            logger.info("stdout: " + result.toString() );
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
    public void getExitCode(RpcClient client)
    {
        Object result = null;

        Vector params = new Vector();

        try {
            result = client.execute("execute.exitValue", params);
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
    public void stopProcess(RpcClient client)
    {
        Object result = null;

        Vector params = new Vector();

        try {
            result = client.execute("execute.stop", params);
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
            logger.info("usage: java " + BootstrapNodeTest.class.getName() + " <connect url>");
            return;
        }
        String urlString = args[0];

        //Create an RpcClient to pass into test methods
        RpcClient client = null;
        try {
            client = new RpcClient(urlString);
        }
        catch (MalformedURLException e) {
            e.printStackTrace();
        }

        //Test #1 run a simple class and then kill it
        BootstrapNodeTest test = new BootstrapNodeTest();

        logger.info("***** starting test: TestContent");
        test.startTestClass(client);
        Thread.sleep(2000);
        test.getStatus(client);
        Thread.sleep(2000);
        test.stopProcess(client);
        Thread.sleep(2000);
        test.getExitCode(client);

        //Test #2 run a full DafNode class and then kill it
        logger.info("\n");
        logger.info("***** starting test: DafNode");
        test.startNode(client);
        Thread.sleep(5000);
        test.getStatus(client);
        Thread.sleep(2000);
        test.stopProcess(client);
        Thread.sleep(2000);
        test.getExitCode(client);
    }

}
