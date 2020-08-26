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
import com.pb.common.daf.Message;
import com.pb.common.daf.MessageProcessingTask;

import java.util.Date;
import java.util.Iterator;

/**
 *Task used to test the DAF framework. This task behaves like a "master"
 * task in that it sends messages to a queue to be processed by another
 * task. After sending three tasks, this task will wait and will exit when
 * the application is stopped.
 *
 * This task extends MessageProcessingTask (which extends Task) and therefore
 * the extender needs only to define the 'onStart' and 'onMessage' methods.  MessageProcessingTask
 * handles the creation of ports.  Unless you require more specific onInit, doWork methods
 * you should be able to extend MessageProcessingTask for your own daf app.
 *
 * @author    Tim Heier
 * @version   1.0, 9/5/2003
 * 
 */
public class SampleMasterMPTask extends MessageProcessingTask {

    
    public void onStart() {
        logger.info( "***" + getName() + " started");
        
        //Send three messages to a queue named "WorkQueue". Each message will 
        //contain two objects, 1) Date and 2) Integer
        for (int i=0; i < 3; i++) {
            Message msg = mFactory.createMessage();
            msg.setId( "testMessage_"+i );
            msg.setValue( "Date", new Date() );
            msg.setValue( "Integer", new Integer(i) );

            //Send the message
            logger.info( getName() + " sending message=" + i );
            sendTo( "WorkQueue", msg );
        }
        
    }
    
    
    public void onMessage(Message msg) {
        logger.info( getName() + " received messageId=" + msg.getId() + " message from=" + msg.getSender() );
        
        //Print out message contents
        int j=0;
        for (Iterator i = msg.valueIterator(); i.hasNext(); ) {
            logger.info( " arg["+ ++j + "]= " + i.next() );
        }
    }

}
