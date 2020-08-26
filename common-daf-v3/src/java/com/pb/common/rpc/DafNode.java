package com.pb.common.rpc;

import org.apache.xmlrpc.*;
import org.apache.log4j.Logger;

import java.io.File;
import java.io.FileNotFoundException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import com.pb.common.util.CommandLine;

public class DafNode
{
    protected static Logger logger = Logger.getLogger(DafNode.class);

    public static int DEFAULT_PORT = 80;
    public static int DEFAULT_BOOTSTRAP_PORT = 8080;

    public String nodeName;
    public int port;
    public NodeConfig nodeConfig;
    public HashMap handlerToNodeMap = new HashMap();
    
    public static String configFile;

    private static DafNode instance = new DafNode();

    private DafNode() {

    }

    /**
     * Returns an instance to the singleton.
     */
    public static DafNode getInstance()
    {
        RPC.setDebug( logger.isDebugEnabled() );
        return instance;
    }

    /**
     * Read node config file
     *
     * @param configFile name of config file to read
     * @return node configuration object
     */
    public static NodeConfig readConfigFile(String configFile) throws Exception
    {
        
        // save configFile name so that handlers instantiated by this class can get the config file
        // name as necessary to create referneces to handlers they need.
        DafNode.configFile = configFile;

        logger.info("reading config file: " + configFile);
        if (configFile == null)
            throw new FileNotFoundException("file name supplied was null");

        NodeConfig nodeConfig = new NodeConfig();
        nodeConfig.readConfig(new File(configFile));

        return nodeConfig;
    }

    public void initClient(String configFile) throws Exception
    {
        this.nodeName = "client-node";

        try {
            this.nodeConfig = DafNode.readConfigFile(configFile);
            buildHandlerToNodeMap();
        } catch (Exception e) {
            logger.fatal(e);
            throw e;
        }
    }

    private int extractPort(String urlAsString) {

        int port = DEFAULT_PORT;

        //extract port
        int begin = urlAsString.lastIndexOf(':');

        if (begin > 0) {
            port = Integer.parseInt(urlAsString.substring(begin+1));
        }

        return port;
    }

    public void startConnectionServer(String nodeName, String configFile)
            throws Exception
    {
        logger.info("starting: " + nodeName);

        this.nodeName = nodeName;

        try {
            this.nodeConfig = DafNode.readConfigFile(configFile);

            String urlAsString = getURL(this.nodeName);
            this.port = extractPort(urlAsString);

            buildHandlerToNodeMap();

            //Create webserver - register default handlers
            ConnectionServer connectionServer = new ConnectionServer(this.port);
            connectionServer.addHandler("math", Math.class);
            connectionServer.addHandler("$default", new Echo());

            //Add SystemHandler, for multicall
            SystemHandler system = new SystemHandler();
            system.addDefaultSystemHandlers();
            connectionServer.addHandler("system", system);

            //Create and register handlers only for this node
            for (int i=0; i < nodeConfig.nHandlers; i++) {
                String handlerName = nodeConfig._handlers[i].name;
                String handlerNode = nodeConfig._handlers[i].node;
                String handlerClass = nodeConfig._handlers[i].className;

                if (nodeName.equalsIgnoreCase(handlerNode)) {
                    Class clazz = Class.forName(handlerClass);
                    connectionServer.addHandler(handlerName, clazz.newInstance());
                }
            }

            //start listening for connections
            connectionServer.start();

            logger.info("Connection server listening on: " + this.port);
        }
        catch (Exception e) {
            logger.fatal(e);
            throw e;
        }
    }

    public void startBootstrapServer(int port, String commandFile) throws Exception
    {
        this.nodeName = "bootstrap-node";
        this.port = port;

        logger.info("starting: " + nodeName);

        //Supply default file name when -command parameter not used 
        if (commandFile == null) {
            commandFile = "startnode.txt";
        }
        try {
            //Create webserver - register default handlers
            ConnectionServer connectionServer = new ConnectionServer(port);
            connectionServer.addHandler("$default", new Echo());

            //Add SystemHandler, for multicall
            SystemHandler system = new SystemHandler();
            system.addDefaultSystemHandlers();
            connectionServer.addHandler("system", system);

            ExecuteHandler executeHandler = new ExecuteHandler();
            executeHandler.setCommandFile(commandFile);

            //Add ExecuteHandler service
            connectionServer.addHandler("execute", executeHandler);

            //start listening for connections
            connectionServer.start();

            logger.info("Using script file: " + commandFile);
            logger.info("Bootstrap server listening on: " + port);
        }
        catch (Exception e) {
            logger.fatal(e);
            throw(e);
        }
    }

    public void buildHandlerToNodeMap()
    {
        //For each handler/endpoint find the NodeSpec associated with the node - need url
        //from node spec to complete handler information
        for (int i=0; i < nodeConfig.nHandlers; i ++) {
            String handlerName = nodeConfig._handlers[i].name;
            String handlerNode    = nodeConfig._handlers[i].node;

            boolean found = false;
            for (int j=0; j < nodeConfig.nNodes; j ++) {
                String currentNode = nodeConfig._nodes[j].name;

                if (currentNode.equalsIgnoreCase(handlerNode)) {

                    //add url of node to handler spec - this completes handler information
                    nodeConfig._handlers[i].url = nodeConfig._nodes[j].url;

                    handlerToNodeMap.put(handlerName, nodeConfig._handlers[i]);
                    found = true;
                    break;
                }
            }
            if (! found) {
                throw new RuntimeException("Could not find a node entry for handler: " + handlerName);
            }
        }
        Iterator it = handlerToNodeMap.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pairs = (Map.Entry)it.next();
            logger.info(pairs.getValue());
        }
    }

    
    /** Returns a String[] of handler names from the config file where the beginning of handler
     * names matches exactly the handlerString passed in.  For example, if "MyHandler" is the
     * string argument, then handlers in the config file named MyHandlerA, MyHandler_1, etc. will
     * both be identified as matching, and an array of at least 2 elements with those names will
     * be returned.  A config file handler named newMyHandler will not be recognized as a match.
     *  
     * @param handlerString
     * @return String[]
     */
    public String[] getHandlerNamesStartingWith (String handlerString) {
        
        String[] tempResult = new String[nodeConfig.nHandlers];
        
        int tempCount = 0;
        for (int i=0; i < nodeConfig.nHandlers; i++) {
            
            // if the config file handler name substring matches the handlerString, then the
            // config file handler name is to be included in the array of Strings returned.
            if ( handlerString.length() <= nodeConfig._handlers[i].name.length() ) {
                String substr = nodeConfig._handlers[i].name.substring(0, handlerString.length());
                if ( substr.equals( handlerString ) ) {
                    tempResult[i] = nodeConfig._handlers[i].name;
                    tempCount++;
                }
            }

        }
        String[] result = null;
        
        if ( tempCount > 0 ) {
            
            // create a result array to be returned dimensioned to the number of handler matches found.
            result = new String[tempCount];
            
            // copy the handler name matches to the result array
            tempCount = 0;
            for (int i=0; i < nodeConfig.nHandlers; i ++) {
                if ( tempResult[i] != null ) {
                    result[tempCount] = tempResult[i];
                    tempCount++;
                }
            }
        }
        return result;
    }
    
    /** Returns true if the handler is defined in the rpc config file and the handler ip address
     *  is the same as that of this VM's.  Returns false if the handler is defined in the rpc
     *  config file, but to an ip address different than this VM's.  Returns null if the handler
     *  is not defined in the rpc config file.
     *  
     * @param handlerString
     * @return Boolean
     */
    public Boolean isHandlerLocal( String handlerString ) {
        
        Boolean result = null;

        // if no config file handler names match handleString, return value will be null. 
        for (int i=0; i < nodeConfig.nHandlers; i ++) {
            
            // if one of the config file handler names matches the handlerString, then return either true or false.
            if ( nodeConfig._handlers[i].name.equals( handlerString ) ) {
                
                if ( nodeConfig._handlers[i].node.equalsIgnoreCase(this.nodeName) )
                    result = true;
                else
                    // otherwise, return false.
                    result = false;
                break;
            }
        }
        return result;
    }
    
    
    /** Returns the URL for a node given the node name, eg. node1
     *
     * @param nodeName
     * @return
     * @throws Exception throws when nodeName is not found
     */
    public String getURL(String nodeName) throws Exception {
        for (int j=0; j < nodeConfig.nNodes; j ++) {
            String currentNode = nodeConfig._nodes[j].name;
            if (currentNode.equalsIgnoreCase(nodeName)) {
                return nodeConfig._nodes[j].url;
            }
        }
        throw new Exception("node="+nodeName+", not found in node list");
    }

    public void setDebug(boolean flag)
    {
        RPC.setDebug(flag);
    }

    private static void usage() {
        System.out.println(
        "\n" +
        "usage: java " + DafNode.class.getName() + " <arguments>\n" +
        "\n" +
        "options:\n" +
        "  -node            name for current node\n" +
        "  -config          name of groovy configuration file\n" +
        "  -bootstrap       flags this VM as a bootstrap node\n" +
        "  -port            used with -bootstrap flag, default="+DafNode.DEFAULT_BOOTSTRAP_PORT+"\n" +
        "  -command         file with command used to start a DAF Node"+"\n" +
        "\n" +
        "Starting a node:\n" +
        "ex 1: java " + DafNode.class.getName() + " -node node1 -config testapp.groovy\n" +
        "\n" +
        "Starting a Bootstrap node:\n" +
        "ex 2: java " + DafNode.class.getName() + " -bootstrap -port 4000\n" +
        "ex 3: java " + DafNode.class.getName() + " -bootstrap -script startNode.cmd\n"
        );
    }

    public static void main(String[] args) throws Exception
    {
        if (args.length < 1) {
            usage();
            return;
        }
        CommandLine cmdline = new CommandLine(args);

        int port = DEFAULT_BOOTSTRAP_PORT;
        if (cmdline.exists("bootstrap")) {
            if (cmdline.exists("port")) {
                port = Integer.parseInt(cmdline.value("port"));
            }

            String commandFile = null;

            if (cmdline.exists("command")) {
                commandFile = cmdline.value("command");
            }

            DafNode.getInstance().startBootstrapServer(port, commandFile);
        }
        else if (cmdline.exists("node")) {
            String nodeName = cmdline.value("node");
            if (nodeName.length() == 0) {
                System.err.println("Error, missing node name. eg. -node node1");
                return;
            }

            if (!cmdline.exists("config")) {
                System.err.println("Error, missing config file name. eg. -config testapp.groovy");
                return;
            }
            String configFile = cmdline.value("config");

            DafNode.getInstance().startConnectionServer(nodeName, configFile);
        }
        else {
            usage();
            return;
        }
    }
}
