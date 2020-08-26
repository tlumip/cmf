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
import java.util.Iterator;

import com.pb.common.daf.*;

/**
 *
 * @author    Tim Heier
 * @version   1.0, 9/5/2003
 * 
 */
public class SampleWorkerMPTask extends MessageProcessingTask {

    
    public void onStart() {
        logger.info( "***" + getName() + " started");
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