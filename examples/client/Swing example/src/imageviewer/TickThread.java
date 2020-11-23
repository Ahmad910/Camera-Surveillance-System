package imageviewer;

public class TickThread extends Thread {
	private Monitor _monitor;
	
	public TickThread(Monitor m) {
		_monitor = m;
	}
	
	public void run() {
		try {
			while (true) {
				Thread.sleep(1000);
				_monitor.tick();
			}
		} catch (InterruptedException e) {
			// Exit
		}
	}
}
