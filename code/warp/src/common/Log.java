package common;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.Date;

public class Log {
    private static String timeStamp() {
        return new Date()+",";
    }
    public static void log(String fileName, String line) throws IOException{
        FileOutputStream fos = new FileOutputStream(fileName, true);
        OutputStreamWriter osw = new OutputStreamWriter(fos, StandardCharsets.UTF_8);
        BufferedWriter writer = new BufferedWriter(osw);
        writer.write(timeStamp() + line);
        //writer.newLine();
        writer.flush();
        writer.close();
        osw.close();
        fos.close();
    }

    public static void logInfo(String line) {
        System.out.println(timeStamp() + line);
    }
    public static void logCriticalInfo(String line) {
        System.out.println("\033[44;37m[Critical]" + timeStamp() + line + "\033[0m");
    }
    public static void logError(String line) {
        System.out.println("\033[40;31m[Error]" + timeStamp() + line + "\033[0m");
    }


    public static void log(String fileName, BufferedReader bReader) throws IOException {
        String line;
        FileOutputStream fos = new FileOutputStream(fileName, true);
        OutputStreamWriter osw = new OutputStreamWriter(fos, StandardCharsets.UTF_8);
        BufferedWriter writer = new BufferedWriter(osw);
        while ((line = bReader.readLine()) != null) {
            //System.out.println(line);
            writer.write(timeStamp() + line);
            writer.newLine();
        }
        writer.flush();
        bReader.close();
        writer.close();
        osw.close();
        fos.close();
    }
}
