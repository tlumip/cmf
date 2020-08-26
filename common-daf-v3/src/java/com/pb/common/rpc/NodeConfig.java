package com.pb.common.rpc;

import groovy.util.GroovyScriptEngine;
import groovy.lang.Binding;

import java.io.File;

public class NodeConfig {

    public final String separatorChar = System.getProperty("file.separator");

    public static final int NODES = 100;
    public static final int HANDLERS = 100;

    public NodeSpec[] _nodes;
    public HandlerSpec[] _handlers;

    public int nNodes;
    public int nHandlers;

    public void readConfig(File groovyFile) throws Exception {

        //----- Initialize memory space
        _nodes = new NodeSpec[NODES];
        for (int i=0; i < NODES; i ++) {
            _nodes[i] = new NodeSpec();
        }

        _handlers = new HandlerSpec[HANDLERS];
        for (int i=0; i < HANDLERS; i++) {
            _handlers[i] = new HandlerSpec();
        }

        //----- Set variables accessed by script
        Binding binding = new Binding();
        binding.setVariable("nodes", _nodes);
        binding.setVariable("handlers", _handlers);

        //----- ExecuteHandler script
        String[] roots = new String[] { getPathToFile(groovyFile) };
        GroovyScriptEngine gse = new GroovyScriptEngine(roots);
        gse.run(groovyFile.getName(), binding);

        nNodes = 0;
        for (int i=0; i < _nodes.length; i ++) {
            if (_nodes[i].name != null && _nodes[i].name.length() > 0) {
                nNodes++;

            }
        }

        nHandlers = 0;
        for (int i=0; i < _handlers.length; i ++) {
            if (_handlers[i].name != null && _handlers[i].name.length() > 0) {
//                System.out.println("handlers["+i+"]="+_handlers[i].name);
//                System.out.println("handlers["+i+"]="+_handlers[i].className);
//                System.out.println("handlers["+i+"]="+_handlers[i].node);
                nHandlers++;
            }
        }
//        System.out.println(nHandlers + " handlers defined in script");
    }

    private String getPathToFile(File file) throws Exception {
        String path = file.getCanonicalPath();
        int index = path.lastIndexOf(separatorChar);
        if (index >= 0) {
            return path.substring(0, index);
        }
        return null;
    }

    /**
     * For testing.
     *
     * @param args
     * @throws Exception
     */
    public static void main(String[] args) throws Exception {
        NodeConfig nodeConfig = new NodeConfig();
        nodeConfig.readConfig(new File("test.groovy"));
    }

}
