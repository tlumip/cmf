import java.net.*;
import java.io.*;

//USAGE: groovy controller.groovy multicast_address port

//INPUTS
InetAddress group = InetAddress.getByName("239.192.0.0");
int port = 4000

println "listening on " + group + ":" + port

new Receiver(group:group, port:port)
