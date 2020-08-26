class Receiver implements Runnable {
    InetAddress group
    int port
    Thread internalThread

    public Receiver() {
        internalThread = new Thread(this)
        internalThread.start()
    }

    public void run() {
        MulticastSocket ms = null
        byte[] buffer = new byte[8192]

        try {
            ms = new MulticastSocket(port)
            ms.joinGroup(group)

            while (true) {
                DatagramPacket dp = new DatagramPacket(buffer, buffer.length)
                ms.receive(dp)
                String s = new String(dp.getData(), 0, dp.getLength())
                println s
            }
        }
        catch (IOException e) {
            println e
        }
        finally {
            if (ms != null) {
                try {
                    ms.leaveGroup(group)
                    ms.close()
                }
                catch (IOException e) {
                }
            }
        }
    }
}
