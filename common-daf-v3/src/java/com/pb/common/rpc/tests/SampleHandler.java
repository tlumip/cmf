package com.pb.common.rpc.tests;

import org.apache.log4j.Logger;

/**
 * @author   Tim Heier
 * @version  Sep 5, 2005
 */
public class SampleHandler {

    protected static Logger logger = Logger.getLogger(SampleHandler.class);

    public String sayHello() {
        logger.info("SampleHander.sayHello method");
        return "hello";
    }

    public Double calculate(double inputValue) {
        logger.info("SampleHandler.calculate: param=" + inputValue);
        return (new Double (inputValue *2) );
    }

    public Double throwRuntimeException(double inputValue) {
        logger.info("SampleHandler.throwRuntimeException: param=" + inputValue);
        throw new RuntimeException("SampleHandler.throwRuntimeException");
    }

    public Double throwXmlRpcRuntimeException(double inputValue) {
        logger.info("SampleHandler.throwXmlRpcException");
        throw new RuntimeException("throwing an exception from: SampleHandler.throwXmlRpcException");
    }

    public Double sendIntArray(int[] inputArray) {
        logger.info("SampleHandler.sendIntArray");
        double sum = 0;
        for (int i=0; i < inputArray.length; i++) {
            sum += inputArray[i];
        }
        return (new Double(sum) );
    }

    public int[] returnIntArray(double inputValue) {
        logger.info("SampleHandler.returnIntArray: param=" + inputValue);
        int[] intArray = { 0, 1, 2 };
        return intArray;
    }

}
