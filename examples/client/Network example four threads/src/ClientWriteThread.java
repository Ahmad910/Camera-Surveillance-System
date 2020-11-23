
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;


public class ClientWriteThread extends Thread {

	private ClientSharedData monitor;
	private byte[] buffer;
	
	public ClientWriteThread(ClientSharedData mon) {
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
					Utils.printBuffer("ClientWriteThread", size, buffer);

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
				// Occurs if there is an error trying to write data,
				// for instance that the connection suddenly closed.
				monitor.setActive(false);
				Utils.println("No connection on client side");
			} catch (InterruptedException e) {
				// Occurs when interrupted
				monitor.shutdown();
				break;
			}
		}
		
		Utils.println("Exiting ClientWriteThread");
	}
}
