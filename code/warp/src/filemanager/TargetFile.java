package filemanager;

import java.io.Serializable;

public class TargetFile extends WarpFile implements Serializable  {

    public final String pattern;
    public final String seed;
    public final double distinguishability;

    public TargetFile(String name, String pattern, String seed, double distinguishability) {
        super(name);
        this.pattern = pattern;
        this.seed = seed;
        this.distinguishability = distinguishability;
    }

}
