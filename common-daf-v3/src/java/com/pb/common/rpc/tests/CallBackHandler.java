package com.pb.common.rpc.tests;

import com.pb.common.rpc.AsyncCallback;

import java.net.URL;

import org.apache.log4j.Logger;

/**
 * Sample call back used during an asynchronous method call. This class implements the
 * Apache AsyncCallback used in the XmlRpc classes.
 *
 * @author   Tim Heier
 * @version  1.0, 9/5/2005
 *
 */
class CallBackHandler implements AsyncCallback {

    protected static Logger logger = Logger.getLogger(CallBackHandler.class);

    public void handleResult(Object result, URL url, String method) {
        logger.info("===> in handleResult: " + Thread.currentThread().getName());
        logger.info("result: " + result.toString());
        logger.info("url: " + url);
        logger.info("method: " + method);
        logger.info("");
    }

    public void handleError(Exception exception, URL url, String method) {
        logger.info("===> in handleError: " + Thread.currentThread().getName());
        logger.info(Thread.currentThread().getName() + " is running");
        exception.printStackTrace();
        logger.info("method: " + method);
        logger.info("url: " + url);
        logger.info("");
    }
}
