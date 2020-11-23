import java.net.ServerSocket;
import java.net.Socket;

public class ServerSharedData {
	private volatile ServerSocket serverSocket;
	private volatile Socket socket;
	private volatile boolean isActive;
	private volatile boolean shutdown;
	
	public ServerSharedData() {
		serverSocket = null;
		socket = null;
		isActive = false;
		shutdown = false;
	}
	
	public synchronized ServerSocket getServerSocket() {
		return serverSocket;
	}
	
	public synchronized void setServerSocket(ServerSocket s) {
		serverSocket = s;
	}
	
	public synchronized Socket getSocket() {
		return socket;
	}
	
	public synchronized void setSocket(Socket s) {
		socket = s;
	}
	
	public synchronized void setActive(boolean active) {
		isActive = active;
		notifyAll();
	}
	
	public synchronized void waitUntilActive() throws InterruptedException {
		while (!isActive) wait();
	}
	
	public synchronized void waitUntilNotActive() throws InterruptedException {
		while (isActive) wait();
	}
	
	public synchronized void shutdown() {
		isActive = false;
		shutdown = true;
		notifyAll();
	}
	
	public synchronized boolean isShutdown() {
		return shutdown;
	}
	
	public synchronized void waitUntilShutdown() throws InterruptedException {
		while (!shutdown) wait();
	}
}
