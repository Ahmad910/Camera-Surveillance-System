
import java.io.IOException;
import java.io.InputStream;

// This thread has the double responsibility of connecting 
// and reading data from the socket
public class ServerReadThread extends Thread {

	private ServerSharedData monitor;
	private byte[] buffer;
	
	public ServerReadThread(ServerSharedData mon) {
		monitor = mon;
		buffer = new byte[8192];
	}
	
	// Send packages of random size to active connections
	public void run() {
		while (!monitor.isShutdown())
		{			
			try {
				monitor.waitUntilActive();

				InputStream is = monitor.getSocket().getInputStream();
				
				// Receive data packages of different sizes
				while (true) {
					// Read header
					int size = Pack.HEAD_SIZE;
					int n = 0;
					while ((n = is.read(buffer, n, size)) > 0) {
						size -= n;
					}
					if (size != 0) break;
					
					// Read payload
					int payloadSize = size = Pack.unpackHeaderSize(buffer);
					n = 0;
					while ((n = is.read(buffer, n+Pack.HEAD_SIZE, size)) > 0) {
						size -= n;
					}
					if (size != 0) break;
					
					// Unpack payload and verify integrity
					Utils.printBuffer("ServerReadThread", payloadSize, buffer);
					Pack.unpackPayloadAndVerifyChecksum(buffer);
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
		
		// No resources to dispose since this is the responsibility
		// of the shutdown thread.
		Utils.println("Exiting ServerReadThread");
	}
}