package com.pb.common.rpc;

import java.util.Hashtable;
import java.util.concurrent.Callable;

public class FindAgents implements Callable<String> {

    public static String BR = "<br/>\n";

    public FindAgents() {
    }

    public String call() throws Exception {
        //System.out.println("FingAgents.class in call() method");

        Controller.getInstance().findAgents();
        Thread.sleep(2000);

        Hashtable agentList = Controller.getInstance().getAgents();

        String response =
            "<html><body>\n" +
            "<h1>Agent List</h1><br/>\n";

        //Each entry in the hashtable is a hashtable representing the values for one agent
        for (Object key : agentList.keySet()) {

            Hashtable h = (Hashtable) agentList.get(key);

            for (Object o : h.keySet()) {
                response += o + ":"+h.get(o) + BR;
            }
            response += "<p>\n";
        }
        response += "</body></html>\n";

        return response;
    }
}
