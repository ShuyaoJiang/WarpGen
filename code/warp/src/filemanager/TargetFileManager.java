package filemanager;

import common.FrameworkExceptions;
import common.Log;

import java.io.Serializable;
import java.util.Iterator;
import java.util.Map;

public class TargetFileManager extends FileManager implements Serializable {

    private double threshold = Double.MAX_VALUE;
    String minName;
    private int topN;

    public TargetFileManager(int topN) {
        this.topN = topN;
    }
    @Override
    public boolean addFile(WarpFile warpFile){
        if(warpFile.getClass() != TargetFile.class) {
            return false;
        }
        TargetFile targetFile = (TargetFile) warpFile;
        if(getAlive() < topN){
            //this means topN is not filled with N items
            Log.logInfo("Add to slow cases: " + targetFile.name);
            super.addFile(targetFile);
            if (targetFile.distinguishability < threshold) {
                //this means targFile.distinguishability is the smallest in all, should update the smallest one
                threshold = targetFile.distinguishability;
                minName = targetFile.name;
            }
        }
        else if(targetFile.distinguishability > threshold) {
            //this means the topN is already filled with N items
            //and the incoming one should substitute the min one
            Iterator<Map.Entry<String, WarpFile>> iterator = getFileIterator();
            TargetFile fileItem;

            try {
                Log.logInfo("Remove from slow cases: " + minName);
                finalizeFile(minName);
            } catch (FrameworkExceptions.TargetNotFoundException e) {
                Log.logError("Bug found ##2, please report!");
                System.exit(1);
            }
            Log.logInfo("Add to slow cases: " + targetFile.name);
            super.addFile(targetFile);


            threshold = Double.MAX_VALUE;
            iterator = getFileIterator();
            while (iterator.hasNext()) {
                //iterate again the obtain the min
                Map.Entry<String, WarpFile> entry = iterator.next();
                fileItem = (TargetFile)entry.getValue();
                if(fileItem.isDead()) {
                    continue;
                }
                if(fileItem.distinguishability < threshold) {
                    threshold = fileItem.distinguishability;
                    minName = fileItem.name;
                }
            }
            //now threshold is the min of TOP N
        }
        else {
            super.addFile(targetFile);
            try {
                Log.logInfo("Not a slow case: " + targetFile.name);
                finalizeFile(targetFile.name);
            } catch (FrameworkExceptions.TargetNotFoundException e) {
                Log.logError("Bug found ##3, please report!");
                System.exit(1);
            }
        }
        return true;
    }
}
