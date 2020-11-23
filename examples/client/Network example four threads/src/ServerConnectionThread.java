
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

// Connecting
public class ServerConnectionThread extends Thread {

	private ServerSharedData monitor;
	private int port;
	
	public ServerConnectionThread(ServerSharedData mon, int port) {
		monitor = mon;
		this.port = port;
	}
	
	// Connect and reconnect if connection is dropped.
	// Send packages of random size.
	public void run() {
		try {
			ServerSocket serverSocket = new ServerSocket(port);
			monitor.setServerSocket(serverSocket);
		} catch (IOException e) {
			// Occurs if the server socket cannot be created
			e.printStackTrace();
			monitor.shutdown();
		}
		
		while (!monitor.isShutdown())
		{
			try {
				// Listen and accepting connections
				Utils.println("Accepting connections on port "+port);
				monitor.setActive(false);
				Socket socket = monitor.getServerSocket().accept();
				Utils.println("Connection");
				
				// Configure socket to immediately send data.
				// This is good for streaming.
				socket.setTcpNoDelay(true);
				
				// Inform monitor there is a connection
				monitor.setSocket(socket);
				monitor.setActive(true);
				
				// Wait until connection drops
				monitor.waitUntilNotActive();
				
			} catch (IOException e) {
				// There is an error accepting connection
				// or setting socket option.
			} catch (InterruptedException e) {
				// Interrupt means shutdown
				monitor.shutdown();
				break;
			}
			
			// Something happened with the connection
			monitor.setActive(false);
			Utils.println("No connection on server side");
			
			// Close the socket before attempting reconnect
			try {
				Socket socket = monitor.getSocket();
				if (socket != null) socket.close();
			} catch (IOException e) {
				// Occurs if there is an error in closing the socket.
			}
		}
		
		// No resources to dispose since this is the responsibility
		// of the write thread.
		Utils.println("Exiting ServerConnectionThread");
	}
}