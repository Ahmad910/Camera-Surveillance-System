package Camera.src;

import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 * Created by RTG on 2017-11-22.
 */
public class CameraGUI extends JFrame implements ActionListener {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private ButtonGroup buttonGroup1;
	private ButtonGroup buttonGroup2;
	private JRadioButton radioButton1 = new JRadioButton("Auto", false);
	private JRadioButton radioButton2 = new JRadioButton("Idle", true);
	private JRadioButton radioButton3 = new JRadioButton("Movie", false);
	private JRadioButton radioButton4 = new JRadioButton("Auto", true);
	private JRadioButton radioButton5 = new JRadioButton("Synch", false);
	private JRadioButton radioButton6 = new JRadioButton("Asynch", false);
	private JPanel panel = new JPanel();
	private JPanel panel2 = new JPanel();
	private ImagePanel imagePanel1 = new ImagePanel(1);
	private ImagePanel imagePanel2 = new ImagePanel(2);
	private JTextField textField1 = new JTextField();
	private JTextField textField2 = new JTextField();
	private JButton button1 = new JButton("Connect camera 1");
	private JButton button2 = new JButton("Connect camera 2");
	private ClientData monitor;
	private JLabel modeDisplay;
	private GUIInput guiInput1;
	private GUIInput guiInput2;

	public CameraGUI(String title, ClientData monitor) {
		super(title);
		setLayout(null);
		this.monitor = monitor;
		configureGUIComponents();
	}
	

	public void showImage(ImageClass image, boolean state, long delay) {
		String temp = readModeFromRadioButtons();
		modeDisplay.setText(temp + (monitor.getSynchronizationStatus()));
		if (image.getCameraId() == 0) {
			imagePanel1.setImage(image, state, delay);
		} else
			imagePanel2.setImage(image, state, delay);
	}

	public void actionPerformed(ActionEvent e) {
		if (e.getSource() == radioButton1) {
			monitor.setModeClient(3);
		} else if (e.getSource() == radioButton2) {
			monitor.setModeClient(1);
		} else if (e.getSource() == radioButton3) {
			monitor.setModeClient(2);
		}else if (e.getSource() == radioButton4) {
			monitor.setSynchronizationStatus("Auto");
		} else if (e.getSource() == radioButton5) {
			monitor.setSynchronizationStatus("synchronised");
		} else if (e.getSource() == radioButton6) {
			monitor.setSynchronizationStatus("asynchronised");
			monitor.flush();
		} 
		else if (e.getSource() == button1) {
			if (textField1.getText() != "") {
				String[] split = textField1.getText().split(":");
				Connection con = new Connection(split[0], Integer.parseInt(split[1]), 0, monitor);
				if (con.connected()) {
					buttonAction(con);
					guiInput1 = new GUIInput(this, monitor, 0);
					guiInput1.start();
				}
			}
		} 
		else if (e.getSource() == button2) {
			if (textField2.getText() != null) {
				String[] split = textField2.getText().split(":");
				Connection con = new Connection(split[0], Integer.parseInt(split[1]), 1, monitor);
				if (con.connected()) {
					buttonAction(con);
					guiInput2 = new GUIInput(this, monitor, 1);
					guiInput2.start();
				}
			}
		}
	}

	private String readModeFromRadioButtons() {
		if (radioButton2.isSelected()) {
			return "Idle: ";
		} else if (radioButton3.isSelected()) {
			return "Movie: ";
		} else
			return "Auto: ";
	}
	
	private void configureGUIComponents() {
		
		imagePanel1.setBounds(50, 50, 450, 600);
		add(imagePanel1);
		imagePanel2.setBounds(500, 50, 450, 600);
		add(imagePanel2);

		radioButton1.addActionListener(this);
		radioButton2.addActionListener(this);
		radioButton3.addActionListener(this);

		panel.add(radioButton1);
		panel.add(radioButton2);
		panel.add(radioButton3);

		panel.setBorder(BorderFactory.createTitledBorder("Choice of mode"));
		panel.setBounds(800, 700, 200, 100);
		add(panel);

		radioButton4.addActionListener(this);
		radioButton5.addActionListener(this);
		radioButton6.addActionListener(this);

		panel2.add(radioButton4);
		panel2.add(radioButton5);
		panel2.add(radioButton6);

		panel2.setBorder(BorderFactory.createTitledBorder("Choice of synchronization"));
		panel2.setBounds(600, 700, 200, 100);
		add(panel2);

		buttonGroup1 = new ButtonGroup();
		buttonGroup1.add(radioButton1);
		buttonGroup1.add(radioButton2);
		buttonGroup1.add(radioButton3);

		buttonGroup2 = new ButtonGroup();
		buttonGroup2.add(radioButton4);
		buttonGroup2.add(radioButton5);
		buttonGroup2.add(radioButton6);

		button1.setBounds(100, 720, 100, 60);
		textField1.setBounds(200, 720, 100, 60);
		button2.setBounds(300, 720, 100, 60);
		textField2.setBounds(400, 720, 100, 60);
		button1.addActionListener(this);
		button2.addActionListener(this);
		add(button1);
		add(button2);
		add(textField1);
		add(textField2);

		modeDisplay = new JLabel();
		modeDisplay.setBounds(410, 0, 400, 60);
		add(modeDisplay);
	}

	private void buttonAction(Connection con) {
			FrameInput fI = new FrameInput(monitor, con.getId(), con);
			fI.start();
			ConfigurationOutput co = new ConfigurationOutput(monitor, con);
			co.start();
			RetrieveMotion rm = new RetrieveMotion(monitor, con);
			rm.start();
		
	}
}
