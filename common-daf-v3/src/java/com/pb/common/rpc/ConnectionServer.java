package com.pb.common.rpc;

import java.io.*;
import java.net.BindException;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.EmptyStackException;
import java.util.Stack;
import java.util.StringTokenizer;

import org.apache.commons.codec.binary.Base64;
import org.apache.xmlrpc.*;
import org.apache.log4j.Logger;
import com.pb.common.rpc.tests.SampleHandler;

/**
 * A minimal tcp connection server that exclusively handles RPC requests.
 *
 * @author   Tim Heier
 * @version  1.0 8/13/2005
 */
public class ConnectionServer implements Runnable
{
    protected static Logger logger = Logger.getLogger(ConnectionServer.class);

    protected static int runnerId = 0;

    protected TcpRpcServer tcprpc;
    protected XmlRpcServer xmlrpc;

    protected ServerSocket serverSocket;
    protected Thread listener;
    protected Stack threadpool;
    protected ThreadGroup runners;

    // Inputs to setupServerSocket()
    private InetAddress address;
    private int port;

    protected static final byte[] ctype = toHTTPBytes("Content-Type: text/xml\r\n");
    protected static final byte[] clength = toHTTPBytes("Content-Length: ");
    protected static final byte[] newline = toHTTPBytes("\r\n");
    protected static final byte[] doubleNewline = toHTTPBytes("\r\n\r\n");
    protected static final byte[] conkeep = toHTTPBytes("Connection: Keep-Alive\r\n");
    protected static final byte[] conclose = toHTTPBytes("Connection: close\r\n");
    protected static final byte[] ok = toHTTPBytes(" 200 OK\r\n");
    protected static final byte[] server = toHTTPBytes("Server: Apache XML-RPC 1.0\r\n");
    protected static final byte[] wwwAuthenticate = toHTTPBytes("WWW-Authenticate: Basic realm=XML-RPC\r\n");

    private static final String HTTP_11 = "HTTP/1.1";
    private static final String STAR = "*";
    /**
     * Creates a web server at the specified port number.
     */
    public ConnectionServer(int port)
    {
        this(port, null);
    }

    /**
     * Creates a web server at the specified port number and IP
     * address.
     */
    public ConnectionServer(int port, InetAddress addr)
    {
        this.address = addr;
        this.port = port;
        this.tcprpc = new TcpRpcServer();
        this.xmlrpc = new XmlRpcServer();
        threadpool = new Stack();
        runners = new ThreadGroup("RPC-Runners");
    }

    /**
     * Returns the US-ASCII encoded byte representation of text for
     * HTTP use (as per section 2.2 of RFC 2068).
     */
    protected static final byte[] toHTTPBytes(String text)
    {
        try
        {
            return text.getBytes("US-ASCII");
        }
        catch (UnsupportedEncodingException e)
        {
            throw new Error(e.getMessage() + ": HTTP requires US-ASCII encoding");
        }
    }

    /**
     * Factory method to manufacture the server socket.  Useful as a
     * hook method for subclasses to override when they desire
     * different flavor of socket (i.e. a <code>SSLServerSocket</code>).
     *
     * @param port
     * @param backlog
     * @param addr If <code>null</code>, binds to
     * <code>INADDR_ANY</code>, meaning that all network interfaces on
     * a multi-homed host will be listening.
     * @exception Exception Error creating listener socket.
     */
    protected ServerSocket createServerSocket(int port, int backlog, InetAddress addr)
            throws Exception
    {
        return new ServerSocket(port, backlog, addr);
    }

    /**
     * Initializes this server's listener socket with the specified
     * attributes, assuring that a socket timeout has been set.  The
     * {@link #createServerSocket(int, int, InetAddress)} method can
     * be overridden to change the flavor of socket used.
     *
     * @see #createServerSocket(int, int, InetAddress)
     */
    private synchronized void setupServerSocket(int backlog)
            throws Exception
    {
        // Since we can't reliably set SO_REUSEADDR in JDK 1.3, try
        // try to (re-)open the server socket several times.  Some OSes
        // (Linux and Solaris, for example), hold on to listener sockets
        // for a brief period of time for security reasons before
        // relinquishing their hold.
        int attempt = 1;
        while (serverSocket == null)
        {
            try
            {
                serverSocket = createServerSocket(port, backlog, address);
            }
            catch (BindException e)
            {
                if (attempt == 10)
                {
                    throw e;
                }

                attempt++;
                Thread.sleep(2000);
            }
        }

        if (XmlRpc.debug)
        {
            StringBuffer msg = new StringBuffer();
            msg.append("Opened XML-RPC server socket for ");
            msg.append(address != null ? address.getHostName() : "localhost");
            msg.append(':').append(port);
            if (attempt > 1)
            {
                msg.append(" after ").append(attempt).append(" tries");
            }
            System.out.println(msg.toString());
        }

        // A socket timeout must be set.
        if (serverSocket.getSoTimeout() <= 0)
        {
            serverSocket.setSoTimeout(4096);
        }
    }

    /**
     * Spawns a new thread which binds this server to the port it's
     * configured to accept connections on.
     *
     * @see #run()
     */
    public void start()
    {
        try
        {
            setupServerSocket(50);
        }
        catch (Exception e)
        {
            listener = null;
            e.printStackTrace();
            throw new RuntimeException(e.getMessage());
        }

        // The listener reference is released upon shutdown().
        if (listener == null)
        {
            listener = new Thread(this, "ConnectionServer");
            listener.start();
        }
    }

    /**
     * Register a handler object with this name. Methods of this objects will be
     * callable over XML-RPC as "name.method".
     */
    public void addHandler(String name, Object target)
    {
        //Add to both servers
        tcprpc.addHandler(name, target);
        xmlrpc.addHandler(name, target);
    }

    /**
     * Adds the bundled handlers to the server.  Called by {@link
     * #main(String[])}.
     */
    protected void addDefaultHandlers() throws Exception
    {
        addHandler("math", Math.class);
        addHandler("$default", new Echo());

        // XmlRpcClients can be used as Proxies in XmlRpcServers which is a
        // cool feature for applets.
        // String url = "http://www.mailtothefuture.com:80/RPC2";
        // addHandler("mttf", new XmlRpcClient(url));

        SystemHandler system = new SystemHandler();
        system.addDefaultSystemHandlers();
        addHandler("system", system);
    }

    /**
     * Remove a handler object that was previously registered with this server.
     */
    public void removeHandler(String name)
    {
        tcprpc.removeHandler(name);
        xmlrpc.removeHandler(name);
    }

    /**
     * Listens for client requests until stopped.  Call {@link
     * #start()} to invoke this method, and {@link #shutdown()} to
     * break out of it.
     *
     * @throws RuntimeException Generally caused by either an
     * <code>UnknownHostException</code> or <code>BindException</code>
     * with the vanilla web server.
     *
     * @see #start()
     * @see #shutdown()
     */
    public void run()
    {
        try
        {
            while (listener != null)
            {
                try
                {
                    Socket socket = serverSocket.accept();
//                    try
//                    {
//                        socket.setTcpNoDelay(true);
//                    }
//                    catch (SocketException socketOptEx)
//                    {
//                        System.err.println(socketOptEx);
//                    }

                    if (RPC.isDebug()) {
                        logger.debug("ConnectionServer.run - accepted socket connection from: " +
                                    socket.getRemoteSocketAddress());
                    }
                    Runner runner = getRunner();
                    runner.handle(socket);
                }
                catch (InterruptedIOException checkState)
                {
                    // Timeout while waiting for a client (from
                    // SO_TIMEOUT)...try again if still listening.
                }
                catch (Exception ex)
                {
                    System.err.println("Exception in XML-RPC listener loop ("+ ex + ").");
                    if (XmlRpc.debug)
                    {
                        ex.printStackTrace();
                    }
                }
                catch (Error err)
                {
                    System.err.println("Error in XML-RPC listener loop ("+ err + ").");
                    err.printStackTrace();
                }
            }
        }
        catch (Exception exception)
        {
            System.err.println("Error accepting XML-RPC connections ("+ exception + ").");
            if (XmlRpc.debug)
            {
                exception.printStackTrace();
            }
        }
        finally
        {
            if (serverSocket != null)
            {
                try
                {
                    serverSocket.close();
                    if (XmlRpc.debug)
                    {
                        System.out.print("Closed XML-RPC server socket");
                    }
                    serverSocket = null;
                }
                catch (IOException e)
                {
                    e.printStackTrace();
                }
            }

            // Shutdown our Runner-based threads
            if (runners != null)
            {
                ThreadGroup g = runners;
                runners = null;
                try
                {
                    g.interrupt();
                }
                catch (Exception e)
                {
                    System.err.println(e);
                    e.printStackTrace();
                }
            }
        }
    }

    /**
     * Stop listening on the server port.  Shutting down our {@link
     * #listener} effectively breaks it out of its {@link #run()}
     * loop.
     *
     * @see #run()
     */
    public synchronized void shutdown()
    {
        // Stop accepting client connections
        if (listener != null)
        {
            Thread l = listener;
            listener = null;
            l.interrupt();
        }
    }

    /**
     *
     */
    protected Runner getRunner()
    {
        try
        {
            return (Runner)threadpool.pop();
        }
        catch (EmptyStackException empty)
        {
            int maxRequests = XmlRpc.getMaxThreads();
            if (runners.activeCount() > XmlRpc.getMaxThreads())
            {
                throw new RuntimeException("System overload: Maximum number " +
                                           "of concurrent requests (" + maxRequests + ") exceeded");
            }
            return new Runner();
        }
    }

    /**
     * Put <code>runner</code> back into {@link #threadpool}.
     *
     * @param runner The instance to reclaim.
     */
    void repoolRunner(Runner runner)
    {
        threadpool.push(runner);
    }

    /**
     * Responsible for handling client connections.
     */
    class Runner implements Runnable
    {
        int instanceId = 0;

        Thread thread;
        Connection con;
        int count;

        public Runner ()
        {
            synchronized (Runner.class) {
                instanceId = runnerId++;
            }
            if (RPC.isDebug()) {
                logger.debug("Runner.init["+instanceId+"]");
            }
        }

        /**
         * Handles the client connection on <code>socket</code>.
         *
         * @param socket The source to read the client's request from.
         */
        public synchronized void handle(Socket socket) throws IOException
        {
            if (RPC.isDebug()) {
                logger.debug("Runner.handle["+instanceId+"] - count=" + count);
            }
            con = new Connection(socket);
            count = 0;
            if (thread == null || !thread.isAlive())
            {
                if (RPC.isDebug()) {
                    logger.debug("Runner.handle["+instanceId+"] - creating new thread");
                }
                thread = new Thread(runners, this, "Runner_"+instanceId);
                thread.start();
            }
            else
            {
                // Wake the thread waiting in our run() method.
                this.notify();
            }
        }

        /**
         * Delegates to <code>con.run()</code>.
         */
        public void run()
        {
            while (con != null && Thread.currentThread() == thread)
            {
                con.run();
                count++;
                con = null;

                if (count > 200 || threadpool.size() > 20)
                {
                    // We're old, or the number of threads in the pool
                    // is large.
                    return;
                }
                synchronized(this)
                {
                    repoolRunner(this);
                    try
                    {
                        this.wait();
                    }
                    catch (InterruptedException ir)
                    {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }
    }

    /**
     *
     */
    class Connection implements Runnable
    {
        private Socket socket;
        private BufferedInputStream input;
        private BufferedOutputStream output;
        private String user, password;
        private Base64 base64Codec;
        byte[] buffer;

        /**
         *
         * @param socket
         * @throws IOException
         */
        public Connection (Socket socket) throws IOException
        {
            logger.debug("Connection.init");

            // set read timeout to 30 seconds
            //socket.setSoTimeout (30000);

            //determine type of connection that should be used, http or tcp
            this.socket = socket;
            input = new BufferedInputStream(socket.getInputStream(), 16384);
            output = new BufferedOutputStream(socket.getOutputStream(), 16384);
        }

        /**
         *
         */
        public void run()
        {
            try
            {
                byte[] first2Bytes = new byte[2];

                //Determine what kind of connection is coming in, http or tcp
                boolean isHttp = isHttpConnection(first2Bytes);
                if (isHttp) {
                    logger.debug("Connection.run - Http connection detected");
                }
                else {
                    logger.debug("Connection.run - Tcp connection detected");
                }

                //Loop keeps connection open - first2Bytes are only available the first time through
                if (isHttp) {
                    handleHttp(first2Bytes);
                }
                else {
                    handleTcp(first2Bytes);
                }
            }
            //Exception was thrown during server processing. Most likely a serialization
            //error. Exceptions thrown by handlers should be in the response object. 
            catch (Exception exception)
            {
                logger.error(exception);
            }
            finally
            {
                try
                {
                    if ((socket != null) && (! RPC.isKeepAlive()))
                    {
                        logger.debug("Connection.run - closing socket and existing thread");
                        socket.close();
                    }
                }
                catch (IOException ignore)
                {
                }
            }
        }

        private void handleHttp(byte[] first2Bytes) throws IOException {
            boolean keepAlive = false;

            do
            {
                // reset user authentication
                user = null;
                password = null;
                String line = readFirstLine(first2Bytes);

                if (RPC.isDebug()) {
                    logger.debug("Connection.handleHttp - line=" + line);
                }

                // Netscape sends an extra \n\r after bodypart, swallow it
                if (line != null && line.length() == 0)
                {
                    line = readLine();
                }
                if (XmlRpc.debug)
                {
                    System.out.println(line);
                }
                int contentLength = -1;

                // tokenize first line of HTTP request
                StringTokenizer tokens = new StringTokenizer(line);
                String method = tokens.nextToken();
                String uri = tokens.nextToken();
                String httpVersion = tokens.nextToken();
                keepAlive = XmlRpc.getKeepAlive() && HTTP_11.equals(httpVersion);
                do
                {
                    line = readLine();
                    if (line != null)
                    {
                        if (XmlRpc.debug)
                        {
                            System.out.println(line);
                        }
                        String lineLower = line.toLowerCase();
                        if (lineLower.startsWith("content-length:"))
                        {
                            contentLength = Integer.parseInt(line.substring(15).trim());
                        }
                        if (lineLower.startsWith("connection:"))
                        {
                            keepAlive = XmlRpc.getKeepAlive() && lineLower.indexOf("keep-alive") > -1;
                        }
                        if (lineLower.startsWith("authorization: basic "))
                        {
                            parseAuth (line);
                        }
                    }
                }
                while (line != null && line.length() != 0);

                if ("POST".equalsIgnoreCase(method))
                {
                    ServerInputStream sin = new ServerInputStream(input, contentLength);
                    try
                    {
                        byte[] result = xmlrpc.execute(sin, user, password);
                        writeResponse(result, httpVersion, keepAlive);
                    }
                    catch (AuthenticationFailed unauthorized)
                    {
                        keepAlive = false;
                        writeUnauthorized(httpVersion, method);
                    }
                }
                else
                {
                    keepAlive = false;
                    writeBadRequest(httpVersion, method);
                }
                output.flush();

                //set this value to null so all bytes will be read from stream second time through
                first2Bytes = null;
            }
            while (keepAlive);

        }

        private void handleTcp(byte[] first2Bytes) throws IOException {


            while (true) {
                int contentLength = TcpRpcTransport.readSize(input, first2Bytes);

                if (RPC.isDebug()) {
                    logger.debug("Connection.handleTcp - " + contentLength + " bytes to read");
                }

                ServerInputStream sin = new ServerInputStream(input, contentLength);
                byte[] result = tcprpc.execute(sin, "nobody", "please");

                TcpRpcTransport.sendBytes(output, result);

                //set this value to null so all bytes will be read from stream second time through
                first2Bytes = null;
            }
        }

        private boolean isHttpConnection(byte[] first2Bytes) {
            try {
                first2Bytes[0] = (byte) input.read();
                first2Bytes[1] = (byte) input.read();

                if ( (first2Bytes[0] == 'P') && (first2Bytes[1] == 'O') ) {
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (Exception e) {

            }

            return true;
        }

        /**
         *
         * @return
         * @throws IOException
         */
        private String readFirstLine(byte[] first2Bytes) throws IOException
        {
            if (buffer == null)
            {
                buffer = new byte[1024];
            }

            //First two bytes have been read so fill the buffer and set the count
            int count = 0;
            if (first2Bytes != null)
            {
                buffer[0] = first2Bytes[0];
                buffer[1] = first2Bytes[1];
                count = 2;
            }

            int next;
            for (;;)
            {
                next = input.read();
                if (next < 0 || next == '\n')
                {
                    break;
                }
                if (next != '\r')
                {
                    buffer[count++] = (byte) next;
                }
                if (count >= buffer.length)
                {
                    throw new IOException("HTTP Header too long");
                }
            }
            return new String(buffer, 0, count);
        }

        /**
         *
         * @return
         * @throws IOException
         */
        private String readLine() throws IOException
        {
            if (buffer == null)
            {
                buffer = new byte[1024];
            }
            int next;
            int count = 0;
            for (;;)
            {
                next = input.read();
                if (next < 0 || next == '\n')
                {
                    break;
                }
                if (next != '\r')
                {
                    buffer[count++] = (byte) next;
                }
                if (count >= buffer.length)
                {
                    throw new IOException("HTTP Header too long");
                }
            }
            return new String(buffer, 0, count);
        }
        /**
         *
         * @param line
         */
        private void parseAuth(String line)
        {
            try
            {
                byte[] c = base64Codec.decode(toHTTPBytes(line.substring(21)));
                String str = new String(c);
                int col = str.indexOf(':');
                user = str.substring(0, col);
                password = str.substring(col + 1);
            }
            catch (Throwable ignore)
            {
            }
        }

        private void writeResponse(byte[] payload, String httpVersion,
                                   boolean keepAlive)
            throws IOException
        {
            output.write(toHTTPBytes(httpVersion));
            output.write(ok);
            output.write(server);
            output.write(keepAlive ? conkeep : conclose);
            output.write(ctype);
            output.write(clength);
            output.write(toHTTPBytes(Integer.toString(payload.length)));
            output.write(doubleNewline);
            output.write(payload);
        }

        private void writeBadRequest(String httpVersion, String httpMethod)
            throws IOException
        {
            output.write(toHTTPBytes(httpVersion));
            output.write(toHTTPBytes(" 400 Bad Request"));
            output.write(newline);
            output.write(server);
            output.write(newline);
            output.write(toHTTPBytes("Method " + httpMethod + " not implemented (try POST)"));
        }

        private void writeUnauthorized(String httpVersion, String httpMethod)
            throws IOException
        {
            output.write(toHTTPBytes(httpVersion));
            output.write(toHTTPBytes(" 401 Unauthorized"));
            output.write(newline);
            output.write(server);
            output.write(wwwAuthenticate);
            output.write(newline);
            output.write(toHTTPBytes("Method " + httpMethod + " requires a " + "valid user name and password"));
        }

    }

    /**
     * Examines command line arguments from <code>argv</code>.  If a
     * port may have been provided, parses that port (exiting with
     * error status if the port cannot be parsed).  If no port is
     * specified, defaults to <code>defaultPort</code>.
     *
     * @param defaultPort The port to use if none was specified.
     */
    protected static int determinePort(String[] argv, int defaultPort)
    {
        int port = defaultPort;
        if (argv.length > 0)
        {
            try
            {
                port = Integer.parseInt(argv[0]);
            }
            catch (NumberFormatException nfx)
            {
                System.err.println("Error parsing port number: " + argv[0]);
                System.err.println("Usage: java " + ConnectionServer.class.getName() + " [port]");
                System.exit(1);
            }
        }
        return port;
    }

    /**
     * For testing.
     */
    public static void main(String[] argv)
    {
        int p = determinePort(argv, 8080);
        //XmlRpc.setDebug (true);
        //XmlRpc.setKeepAlive(true);
        ConnectionServer server = new ConnectionServer(p);

        try
        {
            server.addDefaultHandlers();
            server.addHandler("sample", new SampleHandler());
            server.start();
            System.out.println("Connection server started...");
        }
        catch (Exception e)
        {
            System.err.println("Error running Connection server");
            e.printStackTrace();
            System.exit(1);
        }
    }

}
