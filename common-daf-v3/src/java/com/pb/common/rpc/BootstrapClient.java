package com.pb.common.rpc;

import com.pb.common.util.CommandLine;

import java.util.Vector;
import java.io.IOException;

import org.apache.log4j.Logger;

/**
 * This class...
 *
 * @author   Tim Heier
 * @version  1.0, 2/19/2006
 *
 */
public class BootstrapClient {

    protected static Logger logger = Logger.getLogger(BootstrapClient.class);

    //This value can be overriden on the command-line with the -port switch
    public static int PORT = DafNode.DEFAULT_BOOTSTRAP_PORT;

    /**
     * This method calls the ExecuteHandler.start() method in the bootstrap process.
     * The start() method simply runs the script supplied when the bootstrap
     * process was started.
     *
     */
    public static String startNode(String nodeName) throws Exception
    {
        Object result = new String("");
        Vector params = new Vector();

        String urlString = determineURL(nodeName);

        try {
            RpcClient client = new RpcClient(urlString);
            result = client.execute("execute.start", params);
        }
        catch (RpcException e) {
            logger.error(e);
            throw e;
        }
        catch (IOException e) {
            logger.error(e);
            throw e;
        }

        return result.toString();
    }


    /**
     * This method calls the ExecuteHandler.start() method in the bootstrap process.
     * This is done for each node in the config file. The start() method simply runs
     * the script supplied when the bootstrap process was started.
     *
     */
    public static void startAllNodes() throws Exception
    {
        Object result = new String("");
        Vector params = new Vector();

        NodeSpec[] nodeSpec = DafNode.getInstance().nodeConfig._nodes;

        //loop over all the nodes and call the start method
        for (int i=0; i < nodeSpec.length; i++) {

            String nodeName = nodeSpec[i].name;
            String nodeUrl = nodeSpec[i].url;

            //NodeSpec array is fixed size, skip null entries
            if (nodeName == null)
                continue;

            logger.info("starting: "+nodeName + ", " + nodeUrl);
            try {
                RpcClient client = new RpcClient( determineURL(nodeName) );
                result = client.execute("execute.start", params);
                logger.info("result: " + result);
            }
            catch (RpcException e) {
                logger.error(e);
                throw e;
            }
            catch (IOException e) {
                logger.error(e);
                throw e;
            }
        }

    }


    /**
     * This method calls the ExecuteHandler.stop() method in the bootstrap process.
     * This is done for each node in the config file. The stop() method simply
     * kills the processs started with start().
     *
     */
    public static void stopAllNodes() throws Exception
    {
        Object result = new String("");
        Vector params = new Vector();

        NodeSpec[] nodeSpec = DafNode.getInstance().nodeConfig._nodes;

        //loop over all the nodes and call the start method
        for (int i=0; i < nodeSpec.length; i++) {

            String nodeName = nodeSpec[i].name;
            String nodeUrl = nodeSpec[i].url;

            //NodeSpec array is fixed size, skip null entries
            if (nodeName == null)
                continue;

            logger.info("stopping: "+nodeName + ", " + nodeUrl);
            try {
                RpcClient client = new RpcClient( determineURL(nodeName) );
                result = client.execute("execute.stop", params);
                logger.info("result: " + result);
            }
            catch (RpcException e) {
                logger.error(e);
                throw e;
            }
            catch (IOException e) {
                logger.error(e);
                throw e;
            }
        }

    }


    /**
     * This method calls the ExecuteHandler.stop() method in the bootstrap process.
     * The stop() method simply kills the processs started with start().
     *
     */
    public static String stopNode(String urlString)
    {
        Object result = new String("");

        Vector params = new Vector();

        try {
            RpcClient client = new RpcClient(urlString);
            result = client.execute("execute.stop", params);
        }
        catch (RpcException e) {
            logger.error(e);
        }
        catch (IOException e) {
            logger.error(e);
        }

        return result.toString();
    }


    /**
     * This method calls the ExecuteHandler.stdout() method in the bootstrap process
     * and returns all output written to stdout.
     *
     */
    public static String getStdOut(String urlString)
    {
        Object result = new String("");

        Vector params = new Vector();

        try {
            RpcClient client = new RpcClient(urlString);
            result = client.execute("execute.stdout", params);
        }
        catch (RpcException e) {
            logger.error(e);
        }
        catch (IOException e) {
            logger.error(e);
        }

        return result.toString();
    }


    /**
     * This method calls the ExecuteHandler.stderr() method in the bootstrap process
     * and returns all output written to stderr.
     *
     */
    public static String getStdErr(String urlString)
    {
        Object result = new String("");

        Vector params = new Vector();

        try {
            RpcClient client = new RpcClient(urlString);
            result = client.execute("execute.stderr", params);
        }
        catch (RpcException e) {
            logger.error(e);
        }
        catch (IOException e) {
            logger.error(e);
        }

        return result.toString();
    }


    /**
     * This method is a convenience method and calls both the ExecuteHandler.stdout() and
     * ExecuteHandler.stderr() methods.
     *
     */
    public static String getConsole(String urlString)
    {
        Object result = new String("");

        Vector params = new Vector();

        String str = "";

        try {
            RpcClient client = new RpcClient(urlString);

            result = client.execute("execute.stdout", params);
            str = "stdout: \n" + result.toString() + "// \n";

            result = client.execute("execute.stderr", params);
            str += "stderr: \n" + result.toString() + "//";
        }
        catch (RpcException e) {
            logger.error(e);
        }
        catch (IOException e) {
            logger.error(e);
        }

        return str;
    }

    /**
     * Given an node name, returns a URL. The default port for a bootstrap
     * node is used if one is not supplied.
     *
     * @param nodeName
     * @return
     * @throws Exception
     */
    public static String determineURL(String nodeName) throws Exception {

        String urlString = DafNode.getInstance().getURL(nodeName);

        //extract port from URL
        int begin = urlString.lastIndexOf(':');

        //If port was not supplied then add one
        if (begin < 0) {
            urlString = urlString + ":" + PORT;
        }
        //Replace port in config file
        else {
            urlString = urlString.substring(0, begin+1) + PORT;
        }

        return urlString;
    }

    private static void usage() {
        System.out.println(
        "\n" +
        "usage: java " + BootstrapClient.class.getName() + " <arguments> [options]\n" +
        "\n" +
        "arguments:\n" +
        "  -start       starts specified node\n" +
        "  -startAll    starts all nodes in the supplied config file\n" +
        "  -stop        stops the specified node\n" +
        "  -stopAll     stops all nodes in the supplied config file\n" +
        "  -console     gets the stdout and stderr output\n" +
        "  -config      name of groovy configuration file\n" +
        "\n" +
        "options:\n" +
        "  -port        used with -bootstrap flag, default="+DafNode.DEFAULT_BOOTSTRAP_PORT+"\n" +
        "\n" +
        "Starting a node:\n" +
        "ex 1: " + BootstrapClient.class.getName() + " -start node1 -config testapp.groovy\n" +
        "\n" +
        "Starting all nodes:\n" +
        "ex 2: " + BootstrapClient.class.getName() + " -startAll -config testapp.groovy\n"
        );
    }

/*
    -copyFile
    -copyDir

*/

    public static void main(String[] args) throws Exception
    {
        if (args.length < 1) {
            usage();
            return;
        }
        CommandLine cmdline = new CommandLine(args);

        //----- processing setup commands -----

        //-config is required
        String configFileName = "";
        if (cmdline.exists("config")) {
            configFileName = cmdline.value("config");
            if (configFileName.length() == 0) {
                System.err.println("Error, missing config entry. eg. -config testapp.groovy");
                return;
            }
        }
        else {
            System.err.println("Error, config parameter must be supplied. eg. -config testapp.groovy");
            return;
        }

        //-port is optional
        if (cmdline.exists("port")) {
            String portStr = cmdline.value("port");
            if (portStr.length() == 0) {
                System.err.println("Error, missing port value. eg. -port 8080");
                return;
            }
            PORT = Integer.parseInt(cmdline.value("port"));
        }

        //Read config file and build handler map - nothing else is done yet
        DafNode.getInstance().initClient(configFileName);

        //----- start processing node commands -----

        if (cmdline.exists("start")) {
            String nodeName = cmdline.value("start");
            if (nodeName.length() == 0) {
                System.err.println("Error, missing node name. eg. -start node1");
                return;
            }

            String str = startNode(nodeName);
            logger.info("done");

        }
        else
        if (cmdline.exists("startAll")) {
            startAllNodes();
            logger.info("done");
        }
        else
        if (cmdline.exists("stopAll")) {
            stopAllNodes();
            logger.info("done");
        }
        else
        if (cmdline.exists("stop")) {
            String nodeName = cmdline.value("stop");
            if (nodeName.length() == 0) {
                System.err.println("Error, missing node name. eg. -stop node1");
                return;
            }

            String url = determineURL(nodeName);
            String str = stopNode(url);
            logger.info("done");
        }
        else
        if (cmdline.exists("console")) {
            String nodeName = cmdline.value("console");
            if (nodeName.length() == 0) {
                System.err.println("Error, missing node name. eg. -console node1");
                return;
            }

            String url = determineURL(nodeName);
            String str = getConsole(url);
            logger.info(str);
        }
        else {
            usage();
            return;
        }
    }
}
