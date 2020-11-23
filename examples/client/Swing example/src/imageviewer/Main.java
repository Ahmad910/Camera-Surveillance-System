package imageviewer;

import java.io.File;

import javax.swing.JFileChooser;

public class Main {

	public static void main(String[] args) {
		JFileChooser fc = new JFileChooser();
		fc.setCurrentDirectory(new java.io.File("."));
		fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
		int returnVal = fc.showOpenDialog(null);
		if(returnVal == JFileChooser.APPROVE_OPTION) {
			File folder = fc.getSelectedFile();
			view(folder);
		}
	}

	private static void view(File folder) {
		FileList files = new FileList(folder.getAbsolutePath());
		Monitor m = new Monitor(files);
		MainWindow window = new MainWindow(m);
		TickThread ticker = new TickThread(m);
		ticker.start();
		try {
			ticker.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}
}
