
public class Pack {
	public static final int HEAD_SIZE = 3;
	
	// Pack data into byte array buffer
	// Return number of bytes to send
	public static int pack(byte[] buffer) {
		// Randomize size of package
		int size = (int)(Math.random()*8000+192-HEAD_SIZE);
		
		// Manufacture first two bytes of header (size)
		byte hi = (byte)((size & 0xff00) >>> 8);
		byte lo = (byte)(size & 0xff);
		
		// Set header to buffer
		buffer[0] = hi;
		buffer[1] = lo;
		
		// Manufacture "fake" payload (data)
		// Care is taken to not overwrite header
		for (int i=0; i<size; i++) buffer[i+HEAD_SIZE] = (byte)(i % 255);

		// Manufacture last byte of header (payload checksum)
		int checksum = 0;
		for (int i=0; i<size; i++) checksum += buffer[i+HEAD_SIZE];
		buffer[2] = (byte)(checksum & 0xff);

		// Return number of bytes in buffer to send
		return size + HEAD_SIZE;
	}
	
	// Unpacking is done in two stages. First the header ...
	// Return number of bytes to read
	public static int unpackHeaderSize(byte[] buffer) {
		int hi = (buffer[0] & 0xff) << 8;
		int lo = buffer[1] & 0xff;
		int size = hi | lo;
		return size;
	}
	
	// ... and then the payload
	public static void unpackPayloadAndVerifyChecksum(byte[] buffer) {
		// Verify integrity of received data
		int size = unpackHeaderSize(buffer);
		int checksum = 0;
		for (int i=0; i<size; i++) checksum += buffer[i+HEAD_SIZE];

		if (buffer[2] != (byte)(checksum & 0xff))
			throw new Error("Payload checksum not verified");
		
		// Also need to unpack...
	}
}
