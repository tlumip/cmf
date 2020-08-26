package com.pb.common.rpc;

import org.apache.xmlrpc.XmlRpcClient;
import org.apache.xmlrpc.XmlRpcClientLite;
import org.apache.xmlrpc.XmlRpc;
import org.apache.xmlrpc.WebServer;

import java.net.*;
import java.io.IOException;
import java.util.Vector;
import java.util.Hashtable;

public class NodeAgent implements Runnable {
    private InetAddress group;
    private int multicastPort;
    private int xmlrpcPort;

    private String localIpAddress;

    private Thread internalThread;
    
    public NodeAgent (String multcastAddress, int multicastPort, int xmlrpcPort) throws UnknownHostException {
        this.group = InetAddress.getByName(multcastAddress);
        this.multicastPort = multicastPort;
        this.xmlrpcPort = xmlrpcPort;

        // Get local IP address
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

        startServer();

        internalThread = new Thread(this);
        internalThread.start();
    }

    public void run() {
        System.out.println("NodeAgent listening for multicast traffic on " + group + ":" + multicastPort);

        MulticastSocket ms = null;
        byte[] buffer = new byte[8192];

        try {
            ms = new MulticastSocket(multicastPort);
            ms.joinGroup(group);

            while (true) {
                DatagramPacket dp = new DatagramPacket(buffer, buffer.length);
                ms.receive(dp);
                String s = new String(dp.getData(), 0, dp.getLength());
                processMessage(s);
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
        finally {
            if (ms != null) {
                try {
                    ms.leaveGroup(group);
                    ms.close();
                }
                catch (IOException e) {
                }
            }
        }
    }

    private void processMessage(String data) {
        System.out.println("NodeAgent received broadcast");
        //System.out.println(data);

        String registryAddress = "n/a";
        int registryPort = 0;

        String[] pairs = data.split("#");
        for (String item : pairs) {
            String[] s = item.split(":");
            
            String name = s[0];
            String value = s[1];
            //System.out.println("pairs="+name +":"+value);

            if (name.equalsIgnoreCase("XMLRPC_ADDR")) {
                registryAddress = value;
            }
            else if (name.equalsIgnoreCase("XMLRPC_PORT")) {
                registryPort = Integer.parseInt(value);
            }
        }

        //if (!registered)
        addNodeToRegistry(registryAddress, registryPort);
    }

    /**
     * Add this node to the Registry.
     */
    private void addNodeToRegistry(String ipAddress, int port) {
        XmlRpcClient client = null;

        try {
            XmlRpc.setKeepAlive(false);
            XmlRpc.setDebug(false);
            client = new XmlRpcClientLite("http://" + ipAddress + ":" + port);

            //Register local IP address and port for XML-RPC calls 
            Hashtable h = new Hashtable();
            h.put("XMLRPC_ADDR", localIpAddress);
            h.put("XMLRPC_PORT", xmlrpcPort);

            Vector params = new Vector();
            params.addElement(h);

            Object result = client.execute("registry.register", params);
            System.out.println("Registration result: " + result);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Start XML-RPC server and register this class to receive XML-RPC method calls.
     */
    private void startServer() {
        XmlRpc.setKeepAlive(true);
        XmlRpc.setDebug(false);
        try {
            XmlRpc.setDriver("uk.co.wilson.xml.MinML");
        }
        catch (ClassNotFoundException e) {
            e.printStackTrace();
        }

        // WebServer (contains its own XmlRpcServer instance)
        WebServer webServer = new WebServer(xmlrpcPort);
        webServer.addHandler("NodeAgent", this);
        webServer.start();
        System.out.println("NodeAgent listening for XML-RPC calls on port " + xmlrpcPort);
    }

    public String echo(String message) {
        return "echo: " + message;
    }
    
    public boolean startNode(String data) {
        return true;
    }

    public boolean stopNode(String data) {
        //TODO add implementation
        return true;
    }

    public String getMachineStatus(String data) {
        //TODO add implementation
        return "okay";
    }

    /**
     * Used to run this class from the command-line.
     * 
     * @param args
     * @throws Exception
     */
    public static void main(String[] args) throws Exception {
        XPathEvaluator configFile = new XPathEvaluator("NodeAgentConfig.xml");

        String value = configFile.findValue("/NodeAgentConfig/MulticastAddress");
        String multcastAddress = value;

        value = configFile.findValue("/NodeAgentConfig/MulticastPort");
        int multicastPort = Integer.parseInt(value);

        value = configFile.findValue("/NodeAgentConfig/XmlRpcServerPort");
        int xmlrpcPort = Integer.parseInt(value);

        new NodeAgent(multcastAddress, multicastPort, xmlrpcPort);
    }

}
