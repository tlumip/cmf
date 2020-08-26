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
package com.pb.common.daf.tests;
import com.pb.common.daf.*;

import java.util.Iterator;
import org.apache.log4j.Logger;

/** 
 * Task used to test the DAF framework. This task behaves like a "worker"
 * task in that it reads messages from queue and then processes them.
 * 
 * This task extends Task and so is responsible for creating ports and 
 * pulling messages from queues.
 *
 * @author    Tim Heier
 * @version   1.0, 9/15/2002
 */
public class SampleWorkerTask extends Task {

    Logger logger = Logger.getLogger("com.pb.common.daf");

    public SampleWorkerTask() {
    }


    public SampleWorkerTask(String name) {
        init( name );
    }

    
    public void onStart() {
        logger.info( "***" + getName() + " started");
    }

    
    public void doWork() {

        //Get instance of singleton classes
        PortManager pManager = PortManager.getInstance();
        MessageFactory mFactory = MessageFactory.getInstance();

        //Get a port to communicate with a queue
        Port port = pManager.createPort("WorkQueue");

        //Loop and process messages that arrive
        while (true) {
            Message msg = port.receive();
            logger.info( "Worker task received messageId=" + msg.getId() + " message from=" + msg.getSender() );

            //Print out message contents
            int j=0;
            for (Iterator i = msg.valueIterator(); i.hasNext(); ) {
                logger.info( " arg["+ ++j + "]= " + i.next() );
            }
        }

    }
}