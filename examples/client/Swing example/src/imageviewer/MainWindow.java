package imageviewer;

import java.awt.event.*;
import java.awt.*;

import javax.swing.*;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.concurrent.ExecutionException;


public class MainWindow {
	private Monitor _monitor;
	private JFrame _frame;
	private ImagePanel _imagePanel;
	private InfoPanel _infoPanel;
	private JButton _previousButton;
	private JButton _nextButton;
	
	public MainWindow(Monitor m) {
		_monitor = m;
		initialize();
	}
	
	private void initialize() {
		SwingUtilities.invokeLater(new Runnable() {
			@Override
			public void run() {
				_frame = new JFrame();
				_frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
				WindowAdapter listener = new WindowAdapter() {
					@Override
					public void windowClosing(WindowEvent e) {
						System.exit(0);
					}
				};
				_frame.addWindowListener(listener);

				Container contentPane = _frame.getContentPane();
				contentPane.setLayout(new BoxLayout(contentPane,
						BoxLayout.Y_AXIS));

				_imagePanel = new ImagePanel();
				_infoPanel = new InfoPanel();
				_monitor.setResources(_imagePanel, _infoPanel);
				contentPane.add(_imagePanel);
				contentPane.add(_infoPanel);

				contentPane.add(new JSeparator());

				_previousButton = new JButton("Prev");
				_previousButton.addActionListener(new ActionListener() {
					@Override
					public void actionPerformed(ActionEvent arg0) {
						_monitor.previous();
					}
				});
				_nextButton = new JButton("Next");
				_nextButton.addActionListener(new ActionListener() {
					@Override
					public void actionPerformed(ActionEvent arg0) {
						_monitor.next();
					}
				});
				
				JPanel bottomPane = new JPanel();
				bottomPane.setLayout(new FlowLayout());
				bottomPane.add(_previousButton);
				bottomPane.add(_nextButton);
				contentPane.add(bottomPane);

				_frame.setPreferredSize(new Dimension(410,550));
				_frame.setLocationRelativeTo(null);
				_frame.pack();
				_frame.setVisible(true);
			}
		});
	}
}
