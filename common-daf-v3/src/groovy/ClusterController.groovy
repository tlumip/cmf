import java.net.*
import java.io.*
import com.pb.common.rpc.MiniHTTPD

//USAGE: groovy controller.groovy multicast_address port

//INPUTS
InetAddress group = InetAddress.getByName("239.192.0.0")
int port = 4000
byte ttl = (byte) 2

println "sending on " + group + ":" + port

byte[] data = "name1:value1,name2:value2".getBytes()
DatagramPacket dp = new DatagramPacket(data, data.length, group, port)

//Send broadcast
while (true) {
    try {
        MulticastSocket ms = new MulticastSocket()
        ms.joinGroup(group)
        ms.send(dp, ttl)  //send data
        ms.leaveGroup(group)
        ms.close()
    }
    catch (Exception e) {
        println e
    }
    Thread.sleep(2000);
}
