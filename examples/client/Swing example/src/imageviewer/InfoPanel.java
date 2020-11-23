package imageviewer;

import java.awt.*;
import java.sql.Timestamp;
import javax.swing.*;

public class InfoPanel extends JPanel {
	private JLabel _filename;
	
	public InfoPanel(){
		super();
		setLayout(new GridLayout(2,2));
		_filename = new JLabel("");
		
		add(_filename);
	}

	public void setFilename(String filename){
		_filename.setText(filename);
	}
}
