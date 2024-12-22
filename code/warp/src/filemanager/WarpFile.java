package filemanager;


import java.io.Serializable;

public abstract class WarpFile implements Serializable {
    public final String name;
    private boolean isDead;

    WarpFile(String name) {
        this.name = name;
    }
    public void setDead() {
        isDead = true;
    }

    public boolean isDead() {
        return isDead;
    }

}