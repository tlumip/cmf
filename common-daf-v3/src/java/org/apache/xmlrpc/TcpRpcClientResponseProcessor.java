/*
 * Copyright 1999,2005 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


package org.apache.xmlrpc;

import java.io.InputStream;

import com.pb.common.rpc.TcpRpcTransport;
import com.pb.common.rpc.RPC;
import org.apache.log4j.Logger;

/**
 * Process an XML-RPC server response from a byte array or an
 * InputStream into an Object. Optionally throw the result object
 * if it is an exception.
 *
 * @author <a href="mailto:hannes@apache.org">Hannes Wallnoefer</a>
 * @author <a href="mailto:andrew@kungfoocoder.org">Andrew Evers</a>
 * @version $Id: XmlRpcClientResponseProcessor.java,v 1.4 2005/05/14 21:31:48 jochen Exp $
 * @since 2.0
 */
public class TcpRpcClientResponseProcessor implements RpcClientResponseProcessor
{
    protected static Logger logger = Logger.getLogger(TcpRpcClientResponseProcessor.class);

    /** Set to true if a fault occured on the server. */
    public boolean fault;

    /**
     * Creates a new instance.
     */
    public TcpRpcClientResponseProcessor()
    {
    }

    /**
     * Not used for TCP encoding.
     *
     * @return null
     */
    public String getEncoding() {
        return null;
    }

    /**
     * Decode an XML-RPC response from the specified InputStream.
     *
     * @param is The stream from which to read the response.
     * @return The response, which will be a XmlRpcException if an
     * error occured.
     * @exception XmlRpcClientException
     */
    public Object decodeResponse(InputStream is) throws XmlRpcClientException
    {
        fault = false;
        try
        {
            return readResponse(is);
        }
        catch (Exception x)
        {
            throw new XmlRpcClientException("Error decoding RPC response", x);
        }
    }

    synchronized public Object readResponse(InputStream is) throws Exception
    {
        Object result = TcpRpcTransport.readObject(is);

        if (result instanceof Exception) {
            fault = true;
            if (RPC.isDebug()) {
                logger.debug("TcpRpcClientResponseProcessor.readResponse - result was an exception: "
                            + ((Exception) result).getMessage() );
            }
        }

        return result;
    }

}
