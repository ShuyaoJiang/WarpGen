package common;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;


public class StreamLogger extends Thread {

    BufferedReader bReader = null;
    String fileName = null;
    public StreamLogger(InputStream is, String fileName) {
        try {
            bReader = new BufferedReader(new InputStreamReader(new BufferedInputStream(is), "UTF-8"));
            this.fileName = fileName;
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
    public void run() {
        try {
            Log.log(fileName, bReader);
        } catch (Exception ex) {
            Log.logError("Non-fatal error: Nothing to log.");
        }
    }
}