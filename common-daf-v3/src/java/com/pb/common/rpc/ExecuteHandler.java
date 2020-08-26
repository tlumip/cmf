package com.pb.common.rpc;

import org.apache.log4j.Logger;

import java.util.*;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class ExecuteHandler
{
    protected static Logger logger = Logger.getLogger(ExecuteHandler.class);

    protected Process proc;
    protected StreamReader stderrGobbler;
    protected StreamReader stdoutGobbler;

    protected Thread stderrThread;
    protected Thread stdoutThread;

    protected String commandFile;
    protected Hashtable commandEnv;
    protected Vector commandArgs;

    public ExecuteHandler()
    {
    }

    /**
     * This method is used to run the commandFile which was set when the
     * DafNode class was started.
     *
     * @return message created when the process was started
     */
    public String start()
    {
        //Fills up the commandEnv and commandArgs collections
        readCommandFile();

        return start(commandEnv, commandArgs);
    }

    /**
     * This method is used to an arbitray command on the underlying operating
     * system.
     *
     * @return message created when the process was started
     */
    public String start(Hashtable envVars, Vector args)
    {
        StringBuilder commandLine = new StringBuilder(1024);
        for (int i=0; i < args.size(); i++) {
            commandLine.append(args.elementAt(i));
            commandLine.append(" ");
        }

        logger.info("Execute.start: " + commandLine.toString());

        String returnMessage = "";

        //Add supplied environment variables to process environment
        ProcessBuilder procBuilder = new ProcessBuilder(args);
        Map<String, String> env = procBuilder.environment();

        if (envVars != null) {
            Enumeration keys = envVars.keys();
            while (keys.hasMoreElements()) {
                String key = (String) keys.nextElement();
                String value = (String) envVars.get(key);

                 //add key/value to environment for process
                logger.debug("key="+key+", value="+value);
                env.put(key, value);
            }
        }

        //Start process and capture stderr and stdout streams
        try {
            stop();

            logger.info("Execute.start, starting process");
            proc = procBuilder.start();
            stderrGobbler = new StreamReader("ERR", proc.getErrorStream());
            stdoutGobbler = new StreamReader("OUT", proc.getInputStream());

            stderrThread = new Thread(stderrGobbler);
            stderrThread.start();

            stdoutThread = new Thread(stdoutGobbler);
            stdoutThread.start();
        }
        catch (Exception e) {
            logger.error( e );
            returnMessage = e.getMessage();
        }

        return returnMessage;
    }

    public String stdout()
    {
        String statusMessage = "";

        if ((stdoutGobbler != null) && (stdoutGobbler.getLength() > 0)) {
            statusMessage = stdoutGobbler.getBuffer();
        }
        logger.info("Execute.stdout:");
        if (statusMessage.length() > 0)
            logger.info(statusMessage);
        logger.info("//");

        return statusMessage;
    }

    public String stderr()
    {
        String statusMessage = "";

        if ((stderrGobbler != null) && (stderrGobbler.getLength() > 0)) {
            statusMessage = stderrGobbler.getBuffer();
        }
        logger.info("Execute.stderr:");
        if (statusMessage.length() > 0)
            logger.info(statusMessage);
        logger.info("//");

        return statusMessage;
    }

    public boolean stop()
    {
        logger.info("Execute.stop");

        if (proc != null) {
            proc.destroy();
            proc = null;
        }

        //cleanup stderr
        if (stderrGobbler != null) {
            try {
                stderrGobbler = null;
            } catch (Exception e) {
                //do nothing
            }
        }
        if (stderrThread != null) {
            stderrThread = null;
        }

        //cleanup stdout
        if (stdoutGobbler != null) {
            try {
                stdoutGobbler = null;
            } catch (Exception e) {
                //do nothing
            }
        }
        if (stdoutThread != null) {
            stdoutThread = null;
        }

        return true;
    }

    public int exitValue()
    {
        int exitValue = 0;
        if (proc != null) {
            exitValue = proc.exitValue();
        }
        return exitValue;
    }

    protected void setCommandFile(String commandFile)
    {
        this.commandFile = commandFile;
    }


    protected void readCommandFile() {

        commandArgs = new Vector();
        commandEnv = new Hashtable();

        try {
            BufferedReader in = new BufferedReader(new FileReader(commandFile));
            String str;
            while ((str = in.readLine()) != null) {
                if ( (str.length() < 1) || (str.startsWith("#")) )
                    continue;
                if (str.startsWith("set") || (str.startsWith("SET")) || str.startsWith("Set")) {
                    String tmp = str.substring(4);
                    String[] parts = tmp.split("=");
                    commandEnv.put(parts[0].trim(), parts[1].trim());
                }
                else {
                    commandArgs.add(str);
                }
            }
            in.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    public static void main(String[] args)
    {
        ExecuteHandler exeHandler = new ExecuteHandler();
        exeHandler.start();
    }
}
