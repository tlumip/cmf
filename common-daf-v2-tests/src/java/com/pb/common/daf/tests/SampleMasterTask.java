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

import java.util.Date;
import org.apache.log4j.Logger;

/** 
 * Task used to test the DAF framework. This task behaves like a "master"
 * task in that it sends messages to a queue to be processed by another 
 * task. After sending three tasks, this task exits.
 * 
 * This task extends Task and so is responsible for creating ports and 
 * sending messages to queues.
 *
 * @author    Tim Heier
 * @version   1.0, 9/15/2002
 */
public class SampleMasterTask extends Task {

    Logger logger = Logger.getLogger("com.pb.common.daf");

    
    public SampleMasterTask() {
    }

    
    public SampleMasterTask(String name) {
        init( name );
    }

    
    public void onStart() {
        logger.info( "***" + getName() + " started");
    }

    
    /** Loops and sends three messgaes to another task.
     */
    public void doWork() {

        PortManager pManager = PortManager.getInstance();
        MessageFactory mFactory = MessageFactory.getInstance();

        //Get a port to communicate with a queue
        Port workerPort = pManager.createPort("WorkQueue");

        //Send three messages with a Date and an Integer as the values
        for (int i=0; i < 3; i++) {
            Message msg = mFactory.createMessage();
            msg.setId( "testMessage_"+i );
            msg.setValue( "Date", new Date() );
            msg.setValue( "Integer", new Integer(i) );

            //Send the message
            logger.info( "SampleMasterTask sending message=" + i );
            workerPort.send( msg );
        }
    }

}
