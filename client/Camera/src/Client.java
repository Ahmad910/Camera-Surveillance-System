package Camera.src;

import java.awt.Dimension;

import javax.swing.JFrame;
import javax.swing.WindowConstants;

public class Client {
	public static void main(String args[]) {
		JFrame.setDefaultLookAndFeelDecorated(true);
		ClientData moni = new ClientData();
		CameraGUI gui = new CameraGUI("Camera GUI", moni);
		gui.setVisible(true);
		gui.setSize(new Dimension(1000, 800));
		gui.setResizable(true);
		gui.setDefaultCloseOperation(WindowConstants.EXIT_ON_CLOSE);
	}
}
