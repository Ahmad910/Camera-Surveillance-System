
public class Utils {
	
	public static synchronized void printBuffer(String preamble, int size, byte[] buffer) {
		StringBuffer sb = new StringBuffer();
		sb.append(preamble).append(": ");
		if (size >= 1) {
			sb.append(buffer[0]);
			for (int i=1; i<size; i++) sb.append(",").append(buffer[i]);
		}
		System.out.println(sb.toString());
	}

	public static synchronized void println(String msg) {
		System.out.println(msg);
	}
}
