package filemanager;

import common.FrameworkExceptions;
import java.io.Serializable;
import java.util.Iterator;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public abstract class FileManager implements Serializable {
    protected ConcurrentHashMap<String, WarpFile> files = new ConcurrentHashMap<String, WarpFile>();
    int numDead = 0;

    public boolean exists(String name) {
        return (files.get(name) != null);
    }

    public boolean addFile(WarpFile warpFile){
        if(files.get(warpFile.name) == null) {
            files.put(warpFile.name, warpFile);
            return true;
        }
        return false;
    }
    public int getAlive() {
        //return available size: total size - dead
        return files.size() - numDead;
    }
    public int getSize() {
        //return available size: total size - dead
        return files.size();
    }



    public Iterator<Map.Entry<String, WarpFile>> getFileIterator(){
        return files.entrySet().iterator();
    }

    public void finalizeFile(String name) throws FrameworkExceptions.TargetNotFoundException {
        if (files.get(name) != null) {
            files.get(name).setDead();
            numDead ++;
        }
        else {
            throw new FrameworkExceptions.TargetNotFoundException();
        }
    }

    public WarpFile getFile(String name) {
        return files.get(name);
    }

}
