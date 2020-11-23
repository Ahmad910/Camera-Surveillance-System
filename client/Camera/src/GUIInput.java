package Camera.src;

public class GUIInput extends Thread {
	private long period;
	private long clientDelay;
	private boolean synchronous, asynchronous; // switch �r reserverad
	private ImageClass image;
	private CameraGUI gui;
	private ClientData monitor;
	private long threshold;
	private int counter;
	private long timeToWaitForNextImage;
	private long t;
	private int id;
	private boolean userHasNotPressedOnSynAsyn;

	public GUIInput(CameraGUI gui, ClientData monitor, int id) {
		this.gui = gui;
		this.monitor = monitor;
		period = 2000;
		clientDelay = 0;
		threshold = 200;
		counter = 0;
		synchronous = false;
		image = null;
		timeToWaitForNextImage = 0;
		this.id = id;
		userHasNotPressedOnSynAsyn = true;
		
	}

	public void run() {
		t = System.currentTimeMillis();
		while (!Thread.interrupted()) {
			checkUserSynchronizationChoices();
				if (asynchronous) {
					getImageAndcomputeClientDelay();
					handleAsynchronization();
				} else { // synchronous mode
					getImageAndcomputeClientDelay();
					handleSynchronization();
				}
		}
	}

	/**
	 *  checking which synchronous/asynchronous choices the user choice 
	 *
	 */
	private void checkUserSynchronizationChoices() {
		if (monitor.getSynchronizationStatus().equals("synchronised")) {
			synchronous = true;
			asynchronous = false;
			userHasNotPressedOnSynAsyn = false;

		} else if (monitor.getSynchronizationStatus().equals("asynchronised")) {
			synchronous = false;
			asynchronous = true;
			userHasNotPressedOnSynAsyn = false;
		} else { // it's auto
			asynchronous = !synchronous;
			userHasNotPressedOnSynAsyn = true;
			
		}
	}

	/**
	 * checking if the conditions for changing to synchronous has beet met. If
	 * not continue in asynchronous mode
	 */
	private void handleAsynchronization() {
		gui.showImage(image, asynchronous, clientDelay);
		if (clientDelay > threshold) {
			counter++;
		}
		if (System.currentTimeMillis() >= t + period && counter < 5 && userHasNotPressedOnSynAsyn) {
			t = System.currentTimeMillis();
			synchronous = true;
			monitor.setSynchronizationStatus("synchronised");
			counter = 0;
		} else if (System.currentTimeMillis() >= t + period) {
			t = System.currentTimeMillis();
			counter = 0;
		}
	}

	/**
	 * checking if the conditions for changing to asynchronous has beet met. If
	 * not continue in synchronous mode.
	 */
	private void handleSynchronization() {
		timeToWaitForNextImage = image.getTimestamp() + 200 - System.currentTimeMillis();
		try {
			if (timeToWaitForNextImage >= 0 && userHasNotPressedOnSynAsyn) {
				sleep(timeToWaitForNextImage);
			} else if(userHasNotPressedOnSynAsyn){
				timeToWaitForNextImage = 0;
				counter++;
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		gui.showImage(image, synchronous, clientDelay);
		if (counter == 5 && userHasNotPressedOnSynAsyn) {
			t = System.currentTimeMillis();
			synchronous = false;
			monitor.setSynchronizationStatus("asynchronised");
			counter = 0;
		} else if (System.currentTimeMillis() >= t + period) {
			t = System.currentTimeMillis();
			counter = 0;
		}
	}

	private void getImageAndcomputeClientDelay() {
		image = monitor.getImage(id);
		clientDelay = image.getClientTime() - image.getTimestamp();
	}
}