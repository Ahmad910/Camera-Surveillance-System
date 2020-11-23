
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;

public class ServerWriteThread extends Thread {

	private ServerSharedData monitor;
	private byte[] buffer;
	
	public ServerWriteThread(ServerSharedData mon) {
		monitor = mon;
		buffer = new byte[8192];
	}
	
	// Receive packages of random size from active connections.
	public void run() {
		while (!monitor.isShutdown())
		{			
			try {
				// Blocking wait for connection
				monitor.waitUntilActive();

				Socket socket = monitor.getSocket();
				OutputStream os = socket.getOutputStream();
				
				// Send data packages of different sizes
				while (true) {
					int size = Pack.pack(buffer);
					Utils.printBuffer("ServerWriteThread", size, buffer);
					
					// Send package
					os.write(buffer, 0, size);
					
					// Flush data
					os.flush();

					// "Fake" work done before sending next package
					Thread.sleep(100);
				}
			} catch (IOException e) {
				// Something happened with the connection
				//
				// Example: the connection is closed on the client side, but
				// the server is still trying to read data.
				monitor.setActive(false);
				Utils.println("No connection on server side");
			} catch (InterruptedException e) {
				// Interrupt means shutdown
				monitor.shutdown();
				break;
			}
		}

		Utils.println("Exiting ServerWriteThread");
	}
}
