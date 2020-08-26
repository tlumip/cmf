package com.pb.common.rpc;

import org.apache.xmlrpc.XmlRpc;
import org.apache.xmlrpc.WebServer;

import java.util.Hashtable;
import java.util.ArrayList;
import java.util.HashMap;

public class Registry {
    private int xmlrpcPort;

    //Each entry in the hashtable is a hashtable representing the values for one agent
    //The key is a compound value of ipAddress:port
    private Hashtable agentMap = new Hashtable();

    public Registry(int xmlrpcPort) {
        this.xmlrpcPort = xmlrpcPort;

        startServer();
    }

    private void startServer() {
        XmlRpc.setKeepAlive(false);
        XmlRpc.setDebug(false);
        try {
            XmlRpc.setDriver("uk.co.wilson.xml.MinML");
        }
        catch (ClassNotFoundException e) {
            e.printStackTrace();
        }

        // WebServer (contains its own XmlRpcServer instance)
        WebServer webServer = new WebServer(xmlrpcPort);
        webServer.addHandler("registry", this);
        webServer.start();
        System.out.println("Registry listening for XML-RPC calls on :" + xmlrpcPort);
    }

    public int getXmlrpcPort() {
        return xmlrpcPort;
    }

    public String echo(String message) {
        return "echo: " + message;
    }

    public boolean clearRegistry() {
        agentMap.clear();
        return true;
    }

    public Hashtable getAgentList() {
        return agentMap;
    }

    public boolean register(Hashtable h) {

        System.out.println("Registering agent:");
        for (Object key : h.keySet()) {
            System.out.println(key+":"+h.get(key));
        }

        String ipAddress = (String) h.get("XMLRPC_ADDR");
        int port = (Integer) h.get("XMLRPC_PORT");
        String newKey = ipAddress + ":" + port;

        agentMap.put(newKey, h);

        return true;
    }

    public boolean lookup(String methodName) {
        //TODO add implementation
        return true;
    }
}
