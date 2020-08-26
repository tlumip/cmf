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

import org.xml.sax.*;
import org.w3c.dom.*;
import org.w3c.dom.Node;

import javax.xml.xpath.*;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class XPathEvaluator {

    private String xmlFileName;
    private InputSource inputSource;

    public XPathEvaluator (String xmlFileName) {
        this.xmlFileName = xmlFileName;
        this.inputSource = new InputSource(xmlFileName);
    }

    public String findValue(String searchPath) throws Exception {

        String value = null;

        XPath xpath = XPathFactory.newInstance().newXPath();
        NodeList nodes =
            (NodeList) xpath.evaluate(searchPath, inputSource, XPathConstants.NODESET);

        int length = nodes.getLength();
//        System.out.printf("%d node(s) matched %s", length, args[1]);

        for (int n = 0; n < length; n++) {
            Node node = nodes.item(n);
//            System.out.println(node.getTextContent().replaceAll("\\s+", " "));
//            String nodeName = node.getNodeName();

//            System.out.println("nodeName="+nodeName);

            NamedNodeMap attributes = node.getAttributes();
            Node nameAttribute = attributes.getNamedItem("value");
//            System.out.println("/"+nodeName+"/@value="+nameAttribute.getTextContent());

//            String childSearch = "/application/node[@name='"+nameAttribute.getTextContent()+"']/handler-ref/@ref";
//            System.out.println("childSearch="+childSearch);
//
//            String attribute =
//                (String) xpath.evaluate(childSearch, inputSource, XPathConstants.STRING);
//
//            System.out.println("ref="+ attribute);

            value = nameAttribute.getTextContent();
            break;
        }
        return value;
    }

}
