package filemanager;

import common.FrameworkExceptions;

import java.io.Serializable;
import java.util.*;
import java.util.Map.Entry;

public class SourceFileManager extends FileManager implements Serializable {

    public boolean isGood170(String pattern) {
        return ((SourceFile)files.get(pattern)).isGood170();
    }
    public void setGood170(String pattern) {
        ((SourceFile)files.get(pattern)).setGood170();
    }
    public boolean isSuccessful(String pattern) {
        return ((SourceFile)files.get(pattern)).isSuccessful();
    }
    public void setSuccessful(String pattern) {
        ((SourceFile)files.get(pattern)).setSuccessful();
    }
    public int getFailureCount(String pattern)  {
        return ((SourceFile)files.get(pattern)).getFailureCount();
    }
    public void incFailureCount(String pattern)  {
        ((SourceFile)files.get(pattern)).incFailureCount();
    }

    @Override
    public boolean addFile(WarpFile warpFile){
        if(warpFile.getClass() != SourceFile.class) {
            return false;
        }
        if(exists(warpFile.name)) {
            return false;
        }
        return super.addFile(warpFile);
    }

    public void setMaxDistinguishability(String fileName, double maxDistinguishability) {
        ((SourceFile)files.get(fileName)).setMaxDistinguishability(maxDistinguishability);
    }
    public double getMaxDistinguishability(String fileName) {
        return ((SourceFile)files.get(fileName)).getMaxDistinguishability();
    }

    public String getMaxDistinguishabilityFile(int penaltyUpperBound) throws FrameworkExceptions.TargetNotFoundException {
        String name = null;
        SourceFile warpFile;
        Iterator <Entry<String, WarpFile>> iterator = getFileIterator();
        double maxDistin = 0.0;
        SourceFile retFile = null;
        while (iterator.hasNext()) {
            Entry<String, WarpFile> entry = iterator.next();
            warpFile = (SourceFile)entry.getValue();
            if(warpFile.isDead()){
                //skip useless ones
                continue;
            }
            if(warpFile.getPenalty() > penaltyUpperBound ) {
                //skip those with larger penalty
                continue;
            }
            if(warpFile.getMaxDistinguishability() > maxDistin) {
                retFile = warpFile;
                maxDistin = warpFile.getMaxDistinguishability();
            }
        }
        if(retFile == null) {
            //find nothing
            throw new FrameworkExceptions.TargetNotFoundException();
        }

        Set<SourceFile> retFiles = new HashSet<>();
        Set<SourceFile> retFilesSuccessful = new HashSet<>();
        //如果有一堆相等的
        iterator = getFileIterator();
        while (iterator.hasNext()) {
            Entry<String, WarpFile> entry = iterator.next();
            warpFile = (SourceFile)entry.getValue();
            if(warpFile.isDead()){
                //skip useless ones
                continue;
            }
            if(warpFile.getPenalty() > penaltyUpperBound ) {
                //skip those with larger penalty
                continue;
            }
            if(warpFile.getMaxDistinguishability() == maxDistin) {
                if(warpFile.isSuccessful()) {
                    retFilesSuccessful.add(warpFile);
                }
                retFiles.add(warpFile);
            }
        }
        int size = retFiles.size();
        int sizeSuccessful = retFilesSuccessful.size();
        if(sizeSuccessful >= 1) {
            //优先从成功过的选
            int target = (new Random()).nextInt(size);
            int index = 0;
            Iterator<SourceFile> interatorSet = retFiles.iterator();
            while (interatorSet.hasNext()) {
                retFile = interatorSet.next();
                if (index == target) {
                    break;
                }
                index++;
            }
        }
        else if(size > 1) {
            //然后从没成功过的选，都是170
            int target = (new Random()).nextInt(size);
            int index = 0;
            Iterator<SourceFile> interatorSet = retFiles.iterator();
            while (interatorSet.hasNext()) {
                retFile = interatorSet.next();
                if (index == target) {
                    break;
                }
                index++;
            }
        }
        retFile.inc();
        return retFile.name;
    }



    public String getRandomFile(int penaltyUpperBound) throws FrameworkExceptions.TargetNotFoundException {
        String name = null;
        SourceFile warpFile;
        Iterator <Entry<String, WarpFile>> iterator = getFileIterator();
        //obtain a random index
        int size = getAlive(penaltyUpperBound);
        if(size == 0) {
            throw new FrameworkExceptions.TargetNotFoundException();
        }
        int target = (new Random()).nextInt(size);
        int index = 0;
        while (iterator.hasNext()) {
            Entry<String, WarpFile> entry = iterator.next();
            warpFile = (SourceFile)entry.getValue();
            if(warpFile.isDead()){
                //skip useless ones
                continue;
            }
            if(warpFile.getPenalty() > penaltyUpperBound ) {
                //skip those with larger penalty
                continue;
            }
            if(index == target) {
                name = entry.getKey();
                break;
            }
            index ++;
        }
        if (files.get(name) == null) {
            throw new FrameworkExceptions.TargetNotFoundException();
        }
        ((SourceFile)files.get(name)).inc();
        return name;
    }

    public int getAlive(int penaltyUpperBound) {
        //return available size
        int size = 0;
        SourceFile warpFile;
        Iterator <Map.Entry<String, WarpFile>> iterator = getFileIterator();
        while (iterator.hasNext()) {
            Map.Entry<String, WarpFile> entry = iterator.next();
            warpFile = (SourceFile)entry.getValue();
            if(warpFile.isDead()){
                //skip useless ones
                continue;
            }
            if(warpFile.getPenalty() > penaltyUpperBound ) {
                //skip those with larger penalty
                continue;
            }
            size ++;
        }
        return size;
    }
    public void resetPenalty() {
        SourceFile warpFile;
        Iterator <Map.Entry<String, WarpFile>> iterator = getFileIterator();
        while (iterator.hasNext()) {
            Map.Entry<String, WarpFile> entry = iterator.next();
            warpFile = (SourceFile)entry.getValue();
            if(warpFile.getPenalty() != 0){
                warpFile.reset();
            }
        }
    }
}
