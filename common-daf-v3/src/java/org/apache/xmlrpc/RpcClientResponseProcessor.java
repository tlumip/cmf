package org.apache.xmlrpc;

import java.io.InputStream;

/**
 * @author Tim.Heier
 * @version Oct 15, 2005
 */
public interface RpcClientResponseProcessor {

    public Object decodeResponse(InputStream is) throws XmlRpcClientException;

    public String getEncoding();

}
