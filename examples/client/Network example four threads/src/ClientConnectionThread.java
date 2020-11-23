
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

// This thread has the double responsibility of connecting 
// and reading data from the socket
public class ClientConnectionThread extends Thread {

	private ClientSharedData monitor;
	private String host;
	private int port;
	private boolean reconnect;
	
	public ClientConnectionThread(ClientSharedData mon, String host, int port) {
		monitor = mon;
		this.host = host;
		this.port = port;
		reconnect = false;
	}
	
	// Connect and reconnect if connection is dropped.
	public void run() {
		while (!monitor.isShutdown())
		{
			try {
				// In case of a reconnect, wait some time to avoid busy wait
				if (reconnect) Thread.sleep(1000);
				
				// Establish connection
				Socket socket = new Socket(host, port);
				
				// Configure socket to immediately send data.
				// This is good for streaming.
				socket.setTcpNoDelay(true);
				
				// Inform monitor there is a connection
				monitor.setSocket(socket);
				monitor.setActive(true);
				
				monitor.waitUntilNotActive();
				
			} catch (UnknownHostException e) {
				// Occurs if the socket cannot find the host
			} catch (IOException e) {
				// Something happened with the connection
				//
				// Example: the connection is closed on the server side, but
				// the client is still trying to write data.
				monitor.setActive(false);
				Utils.println("No connection on client side");
			} catch (InterruptedException e) {
				// Occurs when interrupted
				monitor.shutdown();
				break;
			}
			
			// Next connection is a reconnect attempt
			reconnect = true;

			// Close the socket before attempting reconnect
			try {
				Socket socket = monitor.getSocket();
				if (socket != null) socket.close();
			} catch (IOException e) {
				// Occurs if there is an error in closing the socket.
			}
		}
		
		// No resources to dispose since this is the responsibility
		// of the shutdown thread.
		Utils.println("Exiting ClientConnectionThread");
	}
}