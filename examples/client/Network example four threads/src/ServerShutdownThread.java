
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

// This thread has the double responsibility of connecting 
// and reading data from the socket
public class ServerShutdownThread extends Thread {

	private ServerSharedData monitor;
	
	public ServerShutdownThread(ServerSharedData mon) {
		monitor = mon;
	}
	
	// Dispose resources upon shutdown
	public void run() {
		try {
			monitor.waitUntilShutdown();
		} catch (InterruptedException e) {
			// Interrupt also means shutdown
		}
		
		// Close the socket before quitting
		try {
			Socket socket = monitor.getSocket();
			if (socket != null) socket.close();
		} catch (IOException e) {
			// Occurs if there is an error in closing the socket.
		}

		// Close the server socket before quitting
		try {
			ServerSocket serverSocket = monitor.getServerSocket();
			if (serverSocket != null) serverSocket.close();
		} catch (IOException e) {
			// Occurs if there is an error in closing the server socket.
		}
		
		Utils.println("Exiting ServerShutdownThread");
	}
}