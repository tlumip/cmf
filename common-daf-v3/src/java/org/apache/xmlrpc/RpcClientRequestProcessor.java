package org.apache.xmlrpc;

/**
 * @author   Tim.Heier
 * @version  Oct 15, 2005
 */
public interface RpcClientRequestProcessor {

//    void encodeRequest(XmlRpcClientRequest request, String encoding, OutputStream out)
//        throws XmlRpcClientException, IOException;

    byte [] encodeRequestBytes(XmlRpcClientRequest request, String encoding)
        throws XmlRpcClientException;
}
