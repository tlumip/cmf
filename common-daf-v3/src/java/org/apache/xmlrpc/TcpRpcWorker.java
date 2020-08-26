package org.apache.xmlrpc;

import com.pb.common.rpc.TcpRpcTransport;
import com.pb.common.rpc.RPC;
import com.pb.common.util.PerformanceTimer;
import com.pb.common.util.PerformanceTimerType;

import java.io.InputStream;

import org.apache.log4j.Logger;

/**
 * Tie together the XmlRequestProcessor and XmlResponseProcessor to handle
 * a request serially in a single thread.
 *
 */
public class TcpRpcWorker
{
    protected static Logger logger = Logger.getLogger(TcpRpcWorker.class);

    protected XmlRpcHandlerMapping handlerMapping;

    /**
     * Create a new instance that will use the specified mapping.
     */
    public TcpRpcWorker(XmlRpcHandlerMapping handlerMapping)
    {
      this.handlerMapping = handlerMapping;
    }

    /**
     * Pass the specified request to the handler. The handler should be an
     * instance of {@link org.apache.xmlrpc.XmlRpcHandler} or
     * {@link org.apache.xmlrpc.AuthenticatedXmlRpcHandler}.
     *
     * @param handler the handler to call.
     * @param request the request information to use.
     * @param context the context information to use.
     * @return Object the result of calling the handler.
     * @throws ClassCastException if the handler is not of an appropriate type.
     * @throws NullPointerException if the handler is null.
     * @throws Exception if the handler throws an exception.
     */
    protected static Object invokeHandler(Object handler, XmlRpcServerRequest request, XmlRpcContext context)
        throws Exception
    {
        PerformanceTimer pTimer = null;

        try
        {
            if (RPC.isLogPerformance()) {
                pTimer = PerformanceTimer.createNewTimer(request.getMethodName(), PerformanceTimerType.INVOKE);
                pTimer.start();
            }

            if (handler == null)
            {
              throw new NullPointerException ("Null handler passed to XmlRpcWorker.invokeHandler");
            }
            else if (handler instanceof ContextXmlRpcHandler)
            {
                logger.debug("TcpRpcWorker.invokeHandler - instance of ContextXmlRpcHandler");

                return ((ContextXmlRpcHandler) handler).execute
                    (request.getMethodName(), request.getParameters(), context);
            }
            else if (handler instanceof XmlRpcHandler)
            {
                //Pull out Java method name
                String methodName = request.getMethodName();

                int dot = methodName.lastIndexOf('.');
                if (dot > -1 && dot + 1 < methodName.length())
                {
                    methodName = methodName.substring(dot + 1);
                }

                if (RPC.isDebug()) {
                    logger.debug("TcpRpcWorker.invokeHandler - invoking methodName: " + methodName);
                }

                return ((XmlRpcHandler) handler).execute (methodName, request.getParameters());
            }
            else if (handler instanceof AuthenticatedXmlRpcHandler)
            {
                return ((AuthenticatedXmlRpcHandler) handler)
                    .execute(request.getMethodName(), request.getParameters(),
                             context.getUserName(), context.getPassword());
            }
            else
            {
               throw new ClassCastException("HandlerSpec class " +
                                            handler.getClass().getName() +
                                            " is not a valid XML-RPC handler");
            }
        }
        finally
        {
            if (RPC.isLogPerformance()) {
               pTimer.stop();
            }
        }
    }

    /**
     * Decode, process and encode the response or exception for an XML-RPC
     * request. This method executes the handler method with the default context.
     */
    public byte[] execute(InputStream is, String user, String password)
    {
        return execute(is, defaultContext(user, password));
    }

    /**
     * Decode, process and encode the response or exception for an XML-RPC
     * request. This method executes will pass the specified context to the
     * handler if the handler supports context.
     *
     * @param is the InputStream to read the request from.
     * @param context the context for the request (may be null).
     * @return byte[] the response.
     * @throws org.apache.xmlrpc.ParseFailed if the request could not be parsed.
     * @throws org.apache.xmlrpc.AuthenticationFailed if the handler for the
     * specific method required authentication and insufficient credentials were
     * supplied.
     */
    public byte[] execute(InputStream is, XmlRpcContext context)
    {
        try
        {
            XmlRpcServerRequest request = (XmlRpcServerRequest) TcpRpcTransport.readObject(is);
            Object handler = handlerMapping.getHandler(request.getMethodName());
            Object response = invokeHandler(handler, request, context);
            return TcpRpcTransport.encodeBytes(response);
        }
        catch (Exception x)
        {
            if (XmlRpc.debug)
            {
                x.printStackTrace();
            }
            byte[] bytes = null;
            try {
                bytes = TcpRpcTransport.encodeBytes(x);
            } catch (Exception e) {
                logger.error(e);
                //TODO what should be returned to the client - error while serializing an exception
            }
            return bytes;
        }
    }

    /**
     * Factory method to return a default context object for the execute() method.
     * This method can be overridden to return a custom sub-class of XmlRpcContext.
     *
     * @param user the username of the user making the request.
     * @param password the password of the user making the request.
     * @return XmlRpcContext the context for the reqeust.
     */
    protected XmlRpcContext defaultContext(String user, String password)
    {
        return new DefaultXmlRpcContext(user, password, handlerMapping);
    }
}
