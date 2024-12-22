package common;

import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Properties;

public class Configuration {
    private static Configuration conf;
    private static Properties properties;

    public static synchronized Configuration  getInstance(String fileName) throws FrameworkExceptions.ConfigurationException{
        if(conf == null) {
            try {
                properties = new Properties();
                properties.load(new FileReader(fileName));
                conf = new Configuration();
            }
            catch (IOException e)
            {
                Log.logError("Configuration file not found: " + fileName);
                throw new FrameworkExceptions.ConfigurationException();
            }
        }
        return conf;
    }

    private int loadConfInt(String confKey) {
        String text = "Loading " +  confKey;
        int ret = Integer.parseInt(properties.getProperty(confKey));
        text += " = " + ret;
        Log.logInfo(text);
        return ret;

    }
    private String loadConf(String confKey) {
        String text = "Loading " +  confKey;
        String confValue = properties.getProperty(confKey);
        if(confValue == null) {
            throw new NullPointerException();
        }
        text += " = " + confValue;
        Log.logInfo(text);
        return confValue;
    }
    public static synchronized Configuration  getInstance() {
        return conf;
    }

    private String patternFile;
    public String getPatternFile() {
        return patternFile;
    }
    private String seedFile;
    public String getSeedFile() {
        return seedFile;
    }
    private String resultFile;
    public String getResultFile() {
        return resultFile;
    }
    private   int topN;
    public int getTopN() {
        return topN;
    }
    private   String splitter;
    public String getSplitter() {
        return splitter;
    }
    private   int timeToSleepPattern;
    public int getTimeToSleepPattern() {
        return timeToSleepPattern;
    }
    private   String magicStringPattern;
    public String getMagicStringPattern() {
        return magicStringPattern;
    }
    private   String pathPattern;
    public String getPathPattern() {
        return pathPattern;
    }
    private   String tempDir;
    public String getTempDir() {
        return tempDir;
    }
    private    int timeToSleepSeed;
    public int getTimeToSleepSeed() {
        return timeToSleepSeed;
    }
    private   String magicStringSeed;
    public String getMagicStringSeed() {
        return magicStringSeed;
    }
    private   String pathSeed;
    public String getPathSeed() {
        return pathSeed;
    }
    private   int maxPenalty;
    public int getMaxPenalty() {
        return maxPenalty;
    }
    private   String includePath;
    public String getIncludePath() {
        return includePath;
    }
    private   String includePathPattern;
    public String getPatternIncludePath() {
        return includePathPattern;
    }
    private   String emccCommand;
    public String getEmccCommand() {
        return emccCommand;
    }
    private   String insertCommand;
    public String getInsertCommand() {
        return insertCommand;
    }
    private   String wasmEdgeCCommand;
    public String getWasmEdgeCCommand() {
        return wasmEdgeCCommand;
    }
    private   String wasmWamrCCommand;
    public String getWamrCCommand() {
        return wasmWamrCCommand;
    }
    private   String wasmEdgeCommand;
    public String getWasmEdgeCommand() {
        return wasmEdgeCommand;
    }
    private   String wasmerCommand;
    public String getWasmerCommand() {
        return wasmerCommand;
    }
    private   String wasmTimeCommand;
    public String getWasmTimeCommand() {
        return wasmTimeCommand;
    }
    private   String logError;
    public String getLogError() {
        return logError;
    }
    private   String logInfo;
    public String getLogInfo() {
        return logInfo;
    }
    private   String wamrCommand;
    public String getWamrCommand() {
        return wamrCommand;
    }
    private   String logCSVFile;
    public String getLogCSVFile() {
        return logCSVFile;
    }

    private  Configuration() throws FrameworkExceptions.ConfigurationException{
        loadConf();
    }
    public void loadConf() throws FrameworkExceptions.ConfigurationException  {
        try{
            Log.logCriticalInfo("Start to load configurations.");
            //patterns
            magicStringPattern = loadConf("PATTERN_MAGIC_STRING");
            pathPattern = loadConf("PATTERN_PATH");
            magicStringSeed = loadConf("SEED_MAGIC_STRING");
            pathSeed = loadConf("SEED_PATH");
            splitter = loadConf("SPLITTER");
            tempDir = loadConf("TEMP_DIR");
            includePath = loadConf("INCLUDE_PATH");
            includePathPattern = loadConf("INCLUDE_PATH_PATTERN");
            emccCommand = loadConf("EMCC_Command");
            insertCommand = loadConf("INSERT_Command");
            wasmEdgeCCommand = loadConf("WASMEDGEC_Command");
            wasmEdgeCommand = loadConf("WASMEDGE_Command");
            wasmTimeCommand = loadConf("WASMTIME_Command");
            wamrCommand = loadConf("WAMR_Command");
            wasmerCommand = loadConf("WASMER_Command");
            wasmWamrCCommand = loadConf("WAMRC_Command");
            logError = loadConf("LOG_ERROR");
            logInfo = loadConf("LOG_INFO");
            logCSVFile = loadConf("LOG_CSVFILE");
            seedFile = loadConf("SEED_FILE");
            patternFile = loadConf("PATTERN_FILE");
            resultFile = loadConf("RESULT_FILE");
            timeToSleepPattern = loadConfInt("PATTERN_TIME_TO_SLEEP");
            timeToSleepSeed = loadConfInt("SEED_TIME_TO_SLEEP");
            topN = loadConfInt("TOP_N");
            maxPenalty = loadConfInt("MAX_PENALTY");
            Log.logCriticalInfo("Done: Configurations loaded.");
        }
        catch (NumberFormatException | NullPointerException ex){
            Log.logError("Configuration file read error: configure.ini");
            throw new FrameworkExceptions.ConfigurationException();
        }
    }


}
