package Camera.src;

import java.awt.Image;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.*;
import java.nio.file.Files;

import javax.imageio.ImageIO;

public class FrameInput extends Thread {
	private ClientData monitor;
	private int cameraId;
	private Connection con;

	public FrameInput(ClientData monitor, int cameraId, Connection con) {
		this.monitor = monitor;
		this.cameraId = cameraId;
		this.con = con;
	}

	public void run() {
		InputStream inputStream = null;
		ImageClass image;
		long timeStamp;
		try {
			inputStream = con.getInputStream();
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		while (true) {
			try {
				timeStamp = readLong(inputStream);
				int mode = readInt(inputStream);
				int frame_sz = readInt(inputStream);
				byte[] image_bytes = readBytes(inputStream, frame_sz);
				image = new ImageClass(timeStamp, cameraId, image_bytes, mode);
				// The time where the client part gets the image
				image.setClientTime(System.currentTimeMillis());
				monitor.addImage(image);
			} catch (Exception e) {
				String filePath = new File("").getAbsolutePath();
				File disc_img = new File(filePath.concat("/images/disconnected.jpg"));
				System.out.println(disc_img.toPath().toAbsolutePath());
				byte[] fileContent;
				try {
					fileContent = Files.readAllBytes(disc_img.toPath());
					image = new ImageClass(System.currentTimeMillis(), cameraId, fileContent, 0);
					monitor.addImage(image);
				} catch (IOException e1) {
					e1.printStackTrace();
				}
				con.reset();
				try {
					currentThread().join();
				} catch (InterruptedException e1) {
					e1.printStackTrace();
				}
			}
		}
	}

	private int readInt(InputStream is) throws IOException {
		return ByteBuffer.wrap(readBytes(is, 4)).order(ByteOrder.LITTLE_ENDIAN).getInt();
	}

	private long readLong(InputStream is) throws IOException {
		return ByteBuffer.wrap(readBytes(is, 8)).order(ByteOrder.LITTLE_ENDIAN).getLong();
	}

	private byte[] readBytes(InputStream is, int n) throws IOException {
		byte[] data = new byte[n];
		int received = 0;
		do {
			int res = is.read(data, received, n - received);
			if (res >= 0){
			received += res;
			} else {
				throw new IOException();
			}
		} while (received != n);
		return data;
	}
	
}