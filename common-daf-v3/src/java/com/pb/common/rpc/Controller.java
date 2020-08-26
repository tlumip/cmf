package com.pb.common.rpc;

import com.pb.common.http.MiniHTTPD;

import java.io.IOException;
import java.net.InetAddress;
import java.util.Hashtable;

public class Controller {
    private String localIpAddress;

    private Registry registry = null;
    private MiniHTTPD httpd = null;
    private AgentFinder agentFinder = null;

    //The Controller is a singleton
    private static Controller instance = new Controller();

    private Controller() {
    }
    
    /**
     * Returns an instance to the singleton.
     */
    public static Controller getInstance()
    {
        return instance;
    }

    public void init(String multcastAddress, int multicastPort, int xmlrpcPort, int wwwPort, String wwwRoot) throws IOException {

        //Get local IP address
        InetAddress addr = InetAddress.getLocalHost();
        byte[] ipAddr = addr.getAddress();

        //Convert to dot representation
        localIpAddress = "";
        for (int i=0; i<ipAddr.length; i++) {
            if (i > 0) {
                localIpAddress += ".";
            }
            localIpAddress += ipAddr[i]&0xFF;
        }

        registry = new Registry(xmlrpcPort);
        agentFinder = new AgentFinder(multcastAddress, multicastPort, localIpAddress, registry);
        httpd = new MiniHTTPD(wwwPort, wwwRoot);
    }

    public boolean findAgents() {
        agentFinder.findAgents(true);

        return true;
    }

    public Hashtable getAgents() {
        return registry.getAgentList();
    }

    /**
     * Used to run this class from the command-line.
     *
     * @param args
     * @throws Exception
     */
    public static void main(String[] args) throws Exception {

        XPathEvaluator configFile = new XPathEvaluator("ControllerConfig.xml");

        String multcastAddress = configFile.findValue("/ControllerConfig/MulticastAddress");

        String value = configFile.findValue("/ControllerConfig/MulticastPort");
        int multicastPort = Integer.parseInt(value);

        value = configFile.findValue("/ControllerConfig/XmlRpcServerPort");
        int xmlrpcPort = Integer.parseInt(value);

        String wwwRoot = configFile.findValue("/ControllerConfig/WWWContentRoot");

        value = configFile.findValue("/ControllerConfig/WWWServerPort");
        int wwwPort = Integer.parseInt(value);

        Controller.getInstance().init(multcastAddress, multicastPort, xmlrpcPort, wwwPort, wwwRoot);
    }
}
