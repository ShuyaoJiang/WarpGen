package filemanager;

import java.io.Serializable;

public class SourceFile extends WarpFile implements Serializable {
    private int times;
    private int penalty;
    private boolean isGood170 = false;


    public void setGood170() {
        isGood170 = true;
    }
    public boolean isGood170() {
        return isGood170;
    }
    private boolean isSuccessful = false;
    public void setSuccessful() {
        isSuccessful = true;
    }
    public boolean isSuccessful() {
        return isSuccessful;
    }
    double maxDistinguishability = 0.0;
    public void setMaxDistinguishability(double maxDistinguishability) {
        this.maxDistinguishability = maxDistinguishability;
    }
    public double getMaxDistinguishability() {
        return maxDistinguishability;
    }
    private int failureCount = 0;
    public void incFailureCount() {
        failureCount ++;
    }
    public int getFailureCount() {
        return failureCount;
    }

    SourceFile(String name, double maxDistinguishability) {
        super(name);
        this.maxDistinguishability = maxDistinguishability;
    }

    public void inc() {
        times ++;
    }
    public void punish() {
        penalty ++;
    }
    public void setPenalty(int penalty) {
        this.penalty = penalty;
    }
    public void reset() {
        penalty = 0;
    }
    public int getPenalty() {
        return penalty;
    }
}
