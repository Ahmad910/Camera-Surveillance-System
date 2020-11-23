package Camera.src;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import javax.swing.JOptionPane;

public class Connection {

	private String ip;
	private int port;
	private Socket socket;
	private int id;
	private ClientData moni;

	public Connection(String ip, int port, int id, ClientData moni) {
		this.port = port;
		this.ip = ip;
		this.id = id;
		this.moni = moni;
		connect();
	}

	private void connect() {
		try {
			socket = new Socket(ip, port);
			socket.setSoTimeout(6000);
			socket.setTcpNoDelay(true);
			moni.putConnection(this);
		} catch (IOException e) {
			JOptionPane.showMessageDialog(null, "No connection could be found");
		}
	}

	public InputStream getInputStream() throws IOException {
		return socket.getInputStream();
	}

	public OutputStream getOutputStream() throws IOException {
		return socket.getOutputStream();
	}
	
	public String getIP(){
		return this.ip;
	}


	public int getId() {
		return id;
	}

	public boolean connected() {
		return socket != null;
	}
	
	public int getPort(){
		return port;
	}

	public void reset() {
		try {
			socket.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
