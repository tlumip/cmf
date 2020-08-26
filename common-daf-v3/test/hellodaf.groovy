//URLs for each node
nodes[0].name = "node0"
nodes[0].url = "tcp://192.168.2.201:6001"

//nodes[1].name = "node1"
//nodes[1].url = "tcp://192.168.2.202:6001"

//Handler classes, these are the rpc-end points
handlers[0].name = "test"
handlers[0].className = "com.pb.common.rpc.tests.SampleHandler"
handlers[0].node = "node0"

handlers[1].name = "DHashMap"
handlers[1].className = "com.pb.common.rpc.DHashMapHandler"
handlers[1].node = "node0"

handlers[2].name = "DBlockingQueue"
handlers[2].className = "com.pb.common.rpc.DBlockingQueueHandler"
handlers[2].node = "node0"

