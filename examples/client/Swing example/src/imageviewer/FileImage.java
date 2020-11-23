package imageviewer;

import java.io.*;

public class FileImage {
	private byte[] _data;
	private String _path;
	
	
	public FileImage(String filepath) {
		_path = filepath;
		_data = load(_path);
	}
	
	public byte[] getData() {
		return _data;
	}
	
	public String getPath() {
		return _path;
	}
	
	private byte[] load(String path) {
		try {
			File f = new File(path);
			int size = (int)f.length();
			byte[] contents = new byte[size];
			FileInputStream fin = new FileInputStream(f);
			int n = 0;
			while (n < size) {
				n += fin.read(contents, n, size-n);
			}
			fin.close();
			return contents;
		} catch (IOException e) {
			throw new Error(e.getMessage());
		}
	}
}
