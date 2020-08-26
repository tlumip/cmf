/*
 * Copyright  2005 PB Consult Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
package com.pb.common.rpc;

import org.apache.log4j.Logger;

import java.io.*;

class StreamReader implements Runnable
{
    protected static Logger logger = Logger.getLogger(StreamReader.class);

    public static int BUFSIZE = 16384;

    protected String name;
    protected InputStream is;
    protected StringBuilder activeBuffer = new StringBuilder(BUFSIZE);
    protected String storedText = "";

    StreamReader(String name, InputStream is)
    {
        this.name = name;
        this.is = is;
    }

    public String getName() {
        return name;
    }

    public int getLength()
    {
        int length = storedText.length() + activeBuffer.length();
        return length;
    }

    public String getBuffer()
    {
        return storedText.toString() + activeBuffer.toString();
    }

    public void close()
    {
        if (is != null) {
            try {
                is.close();
            } catch (IOException e) {
                //ignore
            } finally {
                is = null;
            }
        }

    }

    public void run()
    {
        int i;

        try {
            InputStreamReader isr = new InputStreamReader(is);
            BufferedReader br = new BufferedReader(isr);

            //store bytes until buffer is full... then discard them
            while ((i = br.read()) != -1) {
                if (activeBuffer.length() == (BUFSIZE-1)) {
                    storedText = activeBuffer.toString();
                    activeBuffer = new StringBuilder(BUFSIZE);
                }
                else {
                    activeBuffer.append((char)i);
                }
            }
        }
        catch (Exception e) {
            logger.error(e);
            return;
        }
    }
}
