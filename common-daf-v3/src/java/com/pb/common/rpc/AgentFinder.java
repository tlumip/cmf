package com.pb.common.rpc;

import com.pb.common.util.BooleanLock;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.net.DatagramPacket;
import java.net.MulticastSocket;

public class AgentFinder implements Runnable {

    private String localIpAddress;
    private String multcastAddress;
    private int multicastPort;
    private Registry registry;

    private InetAddress group;
    private byte ttl = (byte) 2;
    private long sleepTime = 2000;

    private BooleanLock findLock = new BooleanLock(false);
    private Thread internalThread;

    public AgentFinder(String multcastAddress, int multicastPort, String localIpAddress, Registry registry)
            throws UnknownHostException {

        this.multcastAddress = multcastAddress;
        this.multicastPort = multicastPort;
        this.localIpAddress = localIpAddress;
        this.registry = registry;

        this.group = InetAddress.getByName(multcastAddress);

        System.out.println("AgentFinder sending on " + "/" + multcastAddress + ":" + multicastPort);

        internalThread = new Thread(this);
        internalThread.start();
    }

    public void findAgents(boolean shouldFind) {
        if (shouldFind) {
            findLock.setValue(true);
        }
        else {
            findLock.setValue(false);
        }
    }

    public void run() {
        while (true) {
            try {
                //Broadcast once for agents
                findLock.waitUntilTrue(0);
                broadcast();
                findLock.setValue(false);
                //Thread.currentThread().sleep(sleepTime);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Send message as a series of name/values pairs.
     *
     * Format: name1:value1#name2:value2...
     * 
     */
    public void broadcast() {
        //System.out.println("broadcast()");

        StringBuilder sb = new StringBuilder(1024);
        sb.append("XMLRPC_ADDR");
        sb.append(":");
        sb.append(this.localIpAddress);

        sb.append("#");
        sb.append("XMLRPC_PORT");
        sb.append(":");
        sb.append(registry.getXmlrpcPort());

        byte[] data = sb.toString().getBytes();
        DatagramPacket dataPacket = new DatagramPacket(data, data.length, group, multicastPort);

        try {
            MulticastSocket ms = new MulticastSocket();
            ms.joinGroup(group);
            ms.send(dataPacket, ttl);  //send data
            ms.leaveGroup(group);
            ms.close();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

    }

}
