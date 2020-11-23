package imageviewer;

import java.io.*;

public class FileList {
	private String _path;
	private String[] _files;
	
	public FileList(String directorypath) {
		_path = directorypath;
		refresh();
	}
	
	public void refresh() {
		File d = new File(_path);
		File[] files = d.listFiles();
		_files = new String[files.length];
		int i = 0;
		for (File f : files)
			_files[i++] = f.getAbsolutePath();
	}
	
	public String getPath(int i) {
		return _files[i];
	}
	
	public int getSize() {
		return _files.length;
	}
}
