
import java.io.IOException;
import java.net.Socket;


public class ClientShutdownThread extends Thread {

	private ClientSharedData monitor;
	
	public ClientShutdownThread(ClientSharedData mon) {
		monitor = mon;
	}
	
	// Receive packages of random size from active connections.
	public void run() {
		try {
			monitor.waitUntilShutdown();
		} catch (InterruptedException e) {
			// Interrupt means shutdown
		}
		
		// Close the socket before quitting
		try {
			Socket socket = monitor.getSocket();
			if (socket != null) socket.close();
		} catch (IOException e) {
			// Occurs if there is an error in closing the socket.
		}
		Utils.println("Exiting ClientShutdownThread");
	}
}
