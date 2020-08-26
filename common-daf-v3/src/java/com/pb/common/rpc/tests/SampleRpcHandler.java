package com.pb.common.rpc.tests;

import com.pb.common.rpc.RpcHandler;

import java.util.Vector;

import org.apache.log4j.Logger;

/**
 * @author   Tim.Heier
 * @version  1.0, Sep 27, 2005
 */
public class SampleRpcHandler implements RpcHandler {

    protected static Logger logger = Logger.getLogger(SampleRpcHandler.class);

    public Object execute(String methodName, Vector params) throws Exception {

        //Dispatch method calls based on requested methodName
        if (methodName.equalsIgnoreCase("doSomething")) {
            Double param = (Double) params.elementAt(0);
            return doSomething(param.floatValue());
        }
        else
        if (methodName.equalsIgnoreCase("hello")) {
            return sayHello();
        }
        else {
            throw new RuntimeException("methodName="+methodName + " not found");
        }
    }

    private String sayHello() {
        logger.info("hello");
        return "hello";
    }

    private Double doSomething(double inputValue) {
        logger.info("SampleRpcHandler.doSomething: param=" + inputValue);

        //Take a little time
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return (new Double (inputValue *2) );
    }
}
