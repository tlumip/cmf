package org.apache.xmlrpc;
//package org.apache.xmlrpc;

import java.io.InputStream;
import java.io.IOException;
import java.net.URL;

import com.pb.common.util.PerformanceTimer;
import com.pb.common.util.PerformanceTimerType;
import com.pb.common.rpc.TcpRpcTransport;
import com.pb.common.rpc.RPC;
import org.apache.log4j.Logger;

/**
 * Tie together the XmlRequestProcessor and XmlResponseProcessor to handle
 * a request serially in a single thread.
 *
 */
public class XmlRpcClientWorker
{
    protected static Logger logger = Logger.getLogger(XmlRpcClientWorker.class);

    protected static int idCounter = 0;

    protected int instanceId = 0;

    protected URL url;
    protected XmlRpcTransport transport;
    protected RpcClientRequestProcessor requestProcessor;
    protected RpcClientResponseProcessor responseProcessor;

    private static final Object PROCESSING_ERROR_FLAG = new Object();

    public XmlRpcClientWorker(String hostname, int port)
    {
        printInitDebug("creating TcpRpcTransport object");

        this.url = null; // not used
        transport = new TcpRpcTransport(hostname, port);
        requestProcessor = new TcpRpcClientRequestProcessor();
        responseProcessor = new TcpRpcClientResponseProcessor();
    }

    public XmlRpcClientWorker(URL url)
    {
        printInitDebug("creating LiteXmlRpcTransport");

        this.url = url;
        transport = new LiteXmlRpcTransport(url);
        requestProcessor = new XmlRpcClientRequestProcessor();
        responseProcessor = new XmlRpcClientResponseProcessor();
    }


    private void printInitDebug(String message)
    {
        synchronized (XmlRpcClientWorker.class) {
            instanceId = ++idCounter;
        }
        if (RPC.isDebug()) {
            logger.debug("XmlRpcClientWorker.init["+instanceId+"] - " + message);
        }
    }

    /**
     * The response from this method will be an Exception when the remote server throwns
     * an exception.
     *
     * @param xmlRpcRequest
     * @return Object
     * @throws XmlRpcClientException
     * @throws IOException
     */
    synchronized public Object execute(XmlRpcClientRequest xmlRpcRequest)
            throws XmlRpcClientException, IOException
    {
        Object response = PROCESSING_ERROR_FLAG;

        if (RPC.isDebug()) {
            logger.debug("XmlRpcClientWorker.execute["+instanceId+"]");
        }

        PerformanceTimer pTimer = PerformanceTimer.createNewTimer(xmlRpcRequest.getMethodName(),
                                                                    PerformanceTimerType.RPC);
        try {
            pTimer.start();

            byte[] request = requestProcessor.encodeRequestBytes(xmlRpcRequest, responseProcessor.getEncoding());
            InputStream is  = transport.sendXmlRpc(request);
            response = responseProcessor.decodeResponse(is);

            if (RPC.isLogPerformance()) {
                pTimer.stop();
            }

            return response;
        }
        catch (IOException ioe) {
            throw ioe;
        }
        catch (XmlRpcClientException xrce) {
            throw xrce;
        }
        finally {
            if (pTimer.isRunning()) {
                pTimer.cancel();
            }

            // End the transport's session, handling any problem while
            // avoiding hiding of any earlier exception.
            try {
                transport.endClientRequest();
            }
            catch (Throwable t) {
                // Don't clobber an earlier exception.
                boolean haveFault = response instanceof XmlRpcException;
                if (haveFault || response == PROCESSING_ERROR_FLAG) {
                    System.err.println("Avoiding obscuring previous error " +
                                       "by supressing error encountered " +
                                       "while ending request: " + t);
                    if (haveFault) {
                        throw (XmlRpcClientException) response;
                    }
                    // else we've already thrown an exception
                }
                else {
                    if (t instanceof XmlRpcException) {
                        throw (XmlRpcClientException) t;
                    }
                    else {
                        throw new XmlRpcClientException
                            ("Unable to end request", t);
                    }
                }
            }
        }
    }

}
