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

import com.pb.common.util.Convert;

/**
 * Process an RPC client request into an ObjectOutputStream, returning the bytes.
 *
 */
public class TcpRpcClientRequestProcessor implements RpcClientRequestProcessor
{

    public TcpRpcClientRequestProcessor()
    {
    }

    /**
     * Encode a request from the XmlRpcClientRequest implementation to a
     * byte array representing the XML-RPC call, in the specified character
     * encoding.
     *
     * @param request the request to encode.
     * @param encoding the Java name for the encoding to use.
     * @return byte [] the encoded request.
     */
    public byte [] encodeRequestBytes(XmlRpcClientRequest request, String encoding)
            throws XmlRpcClientException
    {

        try {
            return Convert.toBytes(request, 8196);
        }
        catch (Exception e) {
            throw new XmlRpcClientException("Error occured serializing XmlRpcClientRequest to byte[]", e);
        }
    }

}
