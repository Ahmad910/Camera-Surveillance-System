package Camera.src;


public class ImageClass {
	private long timeStamp, clientTime;
	private int cameraId;
	private byte[] data;
	private int mode_id;
	
	public ImageClass(long timeStamp, int id, byte[] d, int mode) {
		this.timeStamp = timeStamp;
		cameraId = id;
		data = d;
		mode_id = mode;


	}
	
	public long getTimestamp() {
		return timeStamp;
	}
	
	public int getCameraId() {
		return cameraId;
	}

	public int getMode() {
		return mode_id;
	}
	
	public byte[] getData() {
		return data;
	}
	
	public long getClientTime() {
	    return clientTime;
	}
	
	public void setClientTime(long clientTime) {
	    this.clientTime = clientTime;
	}
}