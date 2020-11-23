package Camera.src;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URL;

import se.lth.cs.realtime.PeriodicThread;

public class RetrieveMotion extends PeriodicThread {
	private ClientData monitor;
	private Connection con;
	
	public RetrieveMotion(ClientData monitor, Connection con) {
		super(1000);
		this.monitor = monitor;
		this.con = con;
		
	}

	public void perform() {
		URL url;
		HttpURLConnection conn;
		String name = "http://" + con.getIP() + ":" + (con.getPort() + 1);

		System.out.println(name);
		while (!Thread.interrupted()) {
			try {
				url = new URL(name);
				conn = (HttpURLConnection) url.openConnection();
				String temp;
				BufferedReader br = new BufferedReader(new InputStreamReader(conn.getInputStream()));
				temp = br.readLine();
				String[] temp2 = temp.split(":");
				long timestamp = Long.parseLong(temp2[0]);
				if((System.currentTimeMillis() - timestamp*1000) < 10000)
					monitor.setModeServer(2);
			} catch (MalformedURLException e) {
				System.out.println("unable to read the given url.");
			} catch (ProtocolException e) {
				// System.out.println("unable to get the request");
			} catch (IOException e) {
				// System.out.println("unable to open the connection.");
			}
		}
	}
}
