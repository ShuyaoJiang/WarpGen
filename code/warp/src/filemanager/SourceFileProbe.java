package filemanager;

import common.Log;

import java.io.File;
import java.io.FilenameFilter;
import java.util.Date;

public class SourceFileProbe {
    SourceFileManager fileManager;
    String path;
    String magicString;
    public SourceFileProbe(SourceFileManager fileManager, String path, String magicString) {
        this.fileManager = fileManager;
        this.path = path;
        this.magicString = magicString;
    }
    public void doOnce() {
        doOnce(0.0);
    }
    public void doOnce(double distin) {
        File dir = new File(path);
        FilenameFilter filter = new FilenameFilter() {
            public boolean accept(File f, String name) {
                int i = name.lastIndexOf('.');
                if(i > 0) {
                    name = name.substring(i+1);
                }
                return (name.compareToIgnoreCase(magicString) == 0);
            }
        };
        boolean isNewFileFound = false;;

        Log.logCriticalInfo("Starting to probe: " + path);
        //read all files in path
        File[] files = dir.listFiles(filter);
        if(files == null) {
            Log.logError("Target directory not found: " + path);
            return;
        }
        isNewFileFound = false;
        String name = "";
        int n = 0;
        for (int i = 0; i < files.length; i++) {
            //for each file
            String fileName = files[i].getName();
            if(!fileManager.exists(fileName) && fileManager.addFile(new SourceFile(fileName, distin))) {
                name += fileName + ", ";
                n++;
                isNewFileFound = true;
            }
        }
        if (isNewFileFound) {
            Log.logCriticalInfo("Find " + n + " new patterns or seeds: " + name + " distinguishability = " + distin + ". done!");
        }
        else {
            Log.logCriticalInfo("No new pattern or seed.");
        }


    }
}
