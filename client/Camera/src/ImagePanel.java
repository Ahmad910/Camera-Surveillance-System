package Camera.src;


import javax.swing.*;
import javax.swing.border.LineBorder;
import java.awt.*;
import java.text.DecimalFormat;


/**
 * Created by RTG on 2017-11-22.
 */
public class ImagePanel extends JPanel {
	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	private JLabel delay;
	private JLabel mode;
	private JLabel fps;
	private JLabel iconLabel;
	private ImageIcon icon;
	private int counter;
	private long timeStampLastFiveImages;

	private double framesPerSecond;
	private final DecimalFormat formatter = new DecimalFormat("#0.00");

	public ImagePanel(int cameraNumber) {
		this.setOpaque(true);
		iconLabel = new JLabel();
		delay = new JLabel();
		mode = new JLabel();
		fps = new JLabel();
		setLayout(null);
		icon = new ImageIcon();
		setBorder(new LineBorder(Color.gray, 3));
		delay.setVerticalTextPosition(JLabel.CENTER);
		mode.setVerticalTextPosition(JLabel.CENTER);
		fps.setVerticalTextPosition(JLabel.CENTER);
		delay.setText("Delay: ");
		fps.setText("FPS: ");
		mode.setText("Mode: ");

		iconLabel.setIcon(icon);
		iconLabel.setBounds(5, 20, 440, 550);
		delay.setBounds(175, 580, 175, 10);
		mode.setBounds(25, 580, 100, 10);
		fps.setBounds(350, 580, 100, 10);

		add(fps);
		add(mode);
		add(iconLabel);
		add(delay);

		timeStampLastFiveImages = 0;
		framesPerSecond = 0;
	}

	private Image prepare(byte[] data) {
		if (data == null)
			return null;
		Image image = getToolkit().createImage(data);
		Image scaledImage = image.getScaledInstance(440, 540, Image.SCALE_SMOOTH);
		getToolkit().prepareImage(scaledImage, -1, -1, null);
		return scaledImage;
	}

	private void refresh(Image image) {
		if (image == null)
			return;
		icon.setImage(image);
		icon.paintIcon(this, this.getGraphics(), 0, 0);
	}

	public void setImage(ImageClass image, boolean state, long delay2) {
		prepareFPS();
		Image im = prepare(image.getData());
		refresh(im);
		if (delay2 < 0 ){
			delay.setText("Delay: N/A");
		} else {
			delay.setText("Delay: " + delay2);
		}
		
		fps.setText("FPS: " + formatter.format(framesPerSecond));
		switch (image.getMode()) {
		case 1:
			mode.setText("Mode: Idle");
			break;
		case 2:
			mode.setText("Mode: Movie");
			break;
		default:
			mode.setText("Mode: N/A");
		}
	}

	private void prepareFPS() {
		if (timeStampLastFiveImages == 0) {
			timeStampLastFiveImages = System.currentTimeMillis();
		}
		counter++;
		if (counter % 5 == 0) {
			counter = 0;
			long diff = System.currentTimeMillis() - timeStampLastFiveImages;
			framesPerSecond = (1000.0 / diff) * 5;
			timeStampLastFiveImages = System.currentTimeMillis();
		}
		
	}
}
