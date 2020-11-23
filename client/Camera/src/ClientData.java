package Camera.src;

import java.util.*;

public class ClientData {
	public final static int MODE_IDLE = 1, MODE_MOVIE = 2, MODE_AUTO = 3;
	private Queue<ImageClass> queue;
	private Queue<ImageClass> queue2;
	private Map<Integer, Connection> connections;
	private int mode;
	private boolean automaticMode;
	private String currentSynchronization;

	public ClientData() {
		queue = new LinkedList<ImageClass>();
		queue2 = new LinkedList<ImageClass>();
		connections = new HashMap<Integer, Connection>();
		mode = MODE_IDLE;
		automaticMode = false;
		currentSynchronization = "";
	}

	/**
	 * The synchronization to be set by the client or by GUIInput if it's auto
	 * 
	 * @param synchronization
	 *            can be synchronised, asynchronised or auto.
	 */
	public synchronized void setSynchronizationStatus(String synchronization) {
		this.currentSynchronization = synchronization;
	}

	/**
	 * @return the current synchronization status The string to be returned is
	 *         synchronised, asynchronised or auto.
	 */
	public synchronized String getSynchronizationStatus() {
		return currentSynchronization;
	}
	
	/**
	 * Save the success connection in the monitor. 
	 * @param con is the success connection.
	 */
	public synchronized void putConnection(Connection con) {
		connections.put(con.getId(), con);
	}

	/**
	 * The mode to be set by the server
	 * @param mode  movie.
	 */
	public synchronized void setModeServer(int mode) {
		if(automaticMode){
			this.mode = mode;
			notifyAll();
		}
	}

	/**
	 * The mode to be set by the client
	 * @param mode can be idle, movie or auto.
	 */
	public synchronized void setModeClient(int mode) {
		if (mode == MODE_AUTO) {
			automaticMode = true;
		} else {
			automaticMode = false;
			this.mode = mode;

		}
		notifyAll();
	}

	public synchronized int getMode() {
		return mode;
	}

	public synchronized int getModeIfChanged(int oldModeInConfigurationOutput) {
		try {
			while (mode == oldModeInConfigurationOutput) {
				wait();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		notifyAll();
		return mode;
	}

	// add the image to one of the two queues according to which camera the
	// image is sent from.
	public synchronized void addImage(ImageClass image) {

		try {
			while (connections.size() == 0) {
				wait();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		if (image.getCameraId() == 0)
			queue.add(image);
		else
			queue2.add(image);
		notifyAll();
	}

	public synchronized ImageClass getImage(int id) {
		try {
			while ((queue.isEmpty() && id == 0) || (queue2.isEmpty()) && id == 1) {
				wait();
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		if (id == 0)
			return queue.poll();
		else
			return queue2.poll();
	}

	public synchronized void flush() {
		queue.clear();
		queue2.clear();
	}
}
