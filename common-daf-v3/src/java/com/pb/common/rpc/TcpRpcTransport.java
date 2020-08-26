package com.pb.common.rpc;

import org.apache.xmlrpc.XmlRpcTransport;
import org.apache.xmlrpc.XmlRpcClientException;
import org.apache.xmlrpc.XmlRpc;
import org.apache.xmlrpc.ServerInputStream;
import org.apache.log4j.Logger;

import java.io.*;
import java.net.Socket;
import java.net.ConnectException;

import com.pb.common.util.Convert;

public class TcpRpcTransport implements XmlRpcTransport {

    protected static Logger logger = Logger.getLogger(TcpRpcTransport.class);

    String hostname;
    int port;
    String host;
    boolean keepalive = true;

    BufferedOutputStream output;
    BufferedInputStream input;

    Socket socket = null;

    static long bytesSent = 0;
    static long bytesReceived = 0;

    public TcpRpcTransport(String hostname, int port)
    {
        this.hostname = hostname;
        this.port = port;
    }

    public InputStream sendXmlRpc(byte[] request) throws IOException, XmlRpcClientException
    {
        try
        {
            if (socket == null) {
                initConnection();
            }

            InputStream in = null;

           // send request to the server and get an input stream
           // from which to read the response
            try {
                in = sendRequest(request);
            }
            catch (IOException iox) {
                // if we get an exception while sending the request,
                // and the connection is a keepalive connection, it may
                // have been timed out by the server. Try again.

                logger.error("TcpRpcTransport.sendXmlRpc - error while sending request to " +hostname+ ":"+port, iox);

                if (keepalive)
                {
                    closeConnection();
                    initConnection();
                    in = sendRequest(request);
                }
                else {
                    throw iox;
                }
            }

            return in;
        }
        catch (IOException iox)
        {
            // this is a lower level problem,  client could not talk to
            // server for some reason.
            throw iox;
        }
        catch (Exception x)
        {
            // same as above, but exception has to be converted to
            // IOException.
            if (XmlRpc.debug) {
                x.printStackTrace ();
            }

            String msg = x.getMessage ();
            if (msg == null || msg.length () == 0) {
                msg = x.toString ();
            }
            throw new IOException (msg);
        }
    }

    /**
     *
     * @throws IOException
     */
    protected void initConnection() throws IOException
    {
        if (RPC.isDebug()) {
            logger.debug("TcpRpcTransport.initConnection - creating socket to " +hostname+ ":"+port);
        }

        final int retries = 3;
        final int delayMillis = 500;

        int tries = 0;

        socket = null;
        while (socket == null) {
            try {
                socket = new Socket(hostname, port);
            }
            catch (ConnectException e) {
                if (tries >= retries) {
                    throw e;
                } else {
                    logger.error("could not connect to " + hostname + ":" + port +
                                 ", waiting " + new Integer(delayMillis).toString() + "ms and retrying", e);
                    try {
                        Thread.sleep(delayMillis);
                    }
                    catch (InterruptedException ignore) {
                    }
                }
            }
        }

        output = new BufferedOutputStream(socket.getOutputStream(), 16384);
        input = new BufferedInputStream(socket.getInputStream(), 16384);
    }

    /**
     *
     */
    protected void closeConnection ()
    {
        try {
            socket.close();
        }
        catch (Exception ignore) {
        }
        finally {
            socket = null;
        }
    }

    /**
     *
     * @param request
     * @return
     * @throws IOException
     */
    public InputStream sendRequest(byte[] request) throws Exception
    {
        TcpRpcTransport.sendBytes(output, request);
        int bytesInResponse = TcpRpcTransport.readSize(input, null);

        return new ServerInputStream(input, bytesInResponse);
    }

    public void endClientRequest()
    {
        // keepalive is always false if XmlRpc.keepalive is false
        if (!keepalive)
        {
            closeConnection();
        }
    }

    public static void sendBytes(BufferedOutputStream output, byte[] bytes) throws IOException
    {
        int numberOfBytes = bytes.length;

        output.write(Integer.toString(numberOfBytes).getBytes());
        output.write("#".getBytes());
        output.write(bytes);
        output.flush();

        //Keep track of total number of bytes sent
        bytesSent += numberOfBytes;

        if (logger.isDebugEnabled()) {
            logger.debug("TcpRpcTransport.sendBytes - " + numberOfBytes + " bytes");
            logger.debug(">" + Integer.toString(numberOfBytes));
            logger.debug(">" + "#");
            logger.debug(">" + bytes);

        }
    }

    public static byte[] encodeBytes(Object object) throws Exception
    {
        try {
            return Convert.toBytes(object, 8196);
        }
        catch (Exception e) {
            throw new XmlRpcClientException("Error serializing object to byte[]", e);
        }
    }


    public static Object decodeBytes(byte[] bytes) throws Exception {

        try {
            return Convert.toObject(bytes);
        }
        catch (Exception e) {
            throw new XmlRpcClientException("Error serializing object to byte[]", e);
        }
    }

    public static Object readObject(InputStream is) throws IOException
    {
        Object result = null;

        //this is a hack to find the number of bytes that should be read
        int available = 0;
        if (is instanceof ServerInputStream) {
            available = (int) ((ServerInputStream)is).getAvailable();
        } else {
            throw new RuntimeException("InputStream is not an instance of ServerIntputStream");
        }

        byte[] responseBuffer = new byte[available];

        //Read response bytes from server
        int read;
        int start = 0;
        do {
            read = is.read(responseBuffer, start, available);
            start += read;
        } while (read != -1);


        //Convert byte array to object
        result = Convert.toObject(responseBuffer);

        if (RPC.isDebug()) {
            logger.debug("TcpRpcTransport.readObject - " + available + " bytes, " + result.getClass().getName());
        }

        return result;
    }


    /**
     *
     * @return
     * @throws IOException
     */
    public static int readSize(InputStream input, byte[] first2Bytes) throws IOException
    {
        int next = 0;
        int count = 0;

        byte[] buffer = new byte[12];

        if (first2Bytes != null) {
            //Fill first 2 bytes and set count to reflect the next position
            buffer[0] = first2Bytes[0];
            buffer[1] = first2Bytes[1];
            count = 2;

            //Do this incase buffer[1] is equal to '#'
            next = buffer[1];
        }

        while (next != '#') {
            next = input.read();
            if (next < 0) {
                throw new IOException ("unexpected end of stream while reading response size");
            }
            if (next != '#') {
                buffer[count++] = (byte) next;
            }
            if (count >= buffer.length) {
                throw new IOException ("value of response size too large");
            }
        }
        int bytesToRead = Integer.parseInt( new String(buffer, 0, count) );

        //keep track of total bytes read
        bytesReceived += bytesToRead;

        return bytesToRead;
    }


    public long getBytesSent() {
        return bytesSent;
    }

    public long getBytesReceived() {
        return bytesReceived;
    }

    /**
     *
     * @throws Throwable
     */
    protected void finalize() throws Throwable
    {
        closeConnection ();
    }

}
