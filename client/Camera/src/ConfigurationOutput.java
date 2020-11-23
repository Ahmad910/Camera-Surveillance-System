package Camera.src;

import java.io.IOException;
import java.io.OutputStream;


public class ConfigurationOutput extends Thread {
	private ClientData monitor;
	Connection con;

	public ConfigurationOutput(ClientData monitor, Connection con) {
		this.monitor = monitor;
		this.con = con;
	}

	@Override
	public void run() {
		int mode = 1;
		OutputStream outputStream;
		while (true) {
			mode = monitor.getModeIfChanged(mode);
			try {
				outputStream = con.getOutputStream();
				outputStream.write(mode);
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

}