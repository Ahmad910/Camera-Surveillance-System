package imageviewer;

import java.awt.Image;
import java.util.concurrent.ExecutionException;

import javax.swing.SwingWorker;

public class Monitor {
	public final static int WAIT = 5;
	
	private FileList _fileList;
	private ImagePanel _imagePanel;
	private InfoPanel _infoPanel;
	private int _counter;
	private int _index;
	
	public Monitor(FileList fileList) {
		_fileList = fileList;
		_counter = 1;
		_index = 0;
	}
	
	public synchronized void setResources(ImagePanel imagePanel, InfoPanel infoPanel) {
		_imagePanel = imagePanel;
		_infoPanel = infoPanel;
	}
	
	public synchronized void tick() {
		if (_counter <= 0) {
			_index = (_index + 1) % _fileList.getSize();
			viewImage(_fileList.getPath(_index));
			_counter = WAIT;
		}
		_counter--;
	}
	
	public synchronized void previous() {
		_counter = WAIT;
		_index--;
		if (_index < 0) _index = _fileList.getSize()-1;
		viewImage(_fileList.getPath(_index));
	}
	
	public synchronized void next() {
		_counter = WAIT;
		_index = (_index + 1) % _fileList.getSize();
		viewImage(_fileList.getPath(_index));
	}
	
	private class Data {
		public Image image;
		public String path;
		public Data(Image i, String p) {
			image = i;
			path = p;
		}
	}
	
	private void viewImage(String filename) {
		SwingWorker<Data,String> sw = new SwingWorker<Data,String>() {

			@Override
			protected Data doInBackground() throws Exception {
				if (_imagePanel == null) return null;
				FileImage fi = new FileImage(filename);
				Image image = _imagePanel.prepare(fi.getData());
				return new Data(image,filename);
			}
			
			@Override
			protected void done() {
				try {
					if (get() == null) return;
					_imagePanel.refresh(get().image);
					_infoPanel.setFilename(get().path);
				} catch (InterruptedException e) {
					e.printStackTrace();
				} catch (ExecutionException e) {
					e.printStackTrace();
				}
			}
		};
		
		sw.execute();
	}
}
