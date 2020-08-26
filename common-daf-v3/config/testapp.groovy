//URLs for each node
nodes[0].name = "node0"
nodes[0].url = "tcp://localhost:6001"

//Handler classes, these are the rpc-end points
handlers[0].name = "sample"
handlers[0].className = "com.pb.common.rpc.tests.SampleHandler"
handlers[0].node = "node0"

handlers[1].name = "calculator"
handlers[1].className = "com.pb.common.rpc.tests.SampleRpcHandler"
handlers[1].node = "node0"
