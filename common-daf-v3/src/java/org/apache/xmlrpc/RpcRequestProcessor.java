package org.apache.xmlrpc;

import java.io.InputStream;

/**
 * @author   Tim.Heier
 * @version  1.0, Oct 15, 2005
 */
public interface RpcRequestProcessor {

    XmlRpcServerRequest decodeRequest(InputStream is);
}
