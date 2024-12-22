package executor;

import common.Configuration;
import common.FrameworkExceptions;

import java.io.File;
import java.util.Collection;
import java.util.HashMap;

public class Command {
    private static Configuration conf;
    public static final long TIME_OUT = 20000;
    static {
        try {
            conf = Configuration.getInstance("configure.ini");
        } catch (FrameworkExceptions.ConfigurationException e) {
            throw new RuntimeException(e);
        }
    }

    private static String getPrefix(String name) {
        return name.substring(0, name.lastIndexOf("."));
    }

    public static String getSourceFile(String name) {
        return conf.getTempDir() + File.separator + name;
    }


    public static String getSeedFile(String seed) {
        return conf.getPathSeed() + File.separator + seed; //seed文件的位置
    }

    public static String getSeedManagerFile() {
        return conf.getPathSeed() + File.separator + conf.getSeedFile();
    }
    public static String getPatternManagerFile() {
        return conf.getPathPattern() + File.separator + conf.getPatternFile();
    }
    public static String getResultManagerFile() {
        return conf.getTempDir() + File.separator + conf.getResultFile();
    }

    public static String getPatternFile(String pattern) {
        return conf.getPathPattern() + File.separator + pattern; //pattern文件的位置
    }


    public static String getCTargetFile(String resultFile) {
        return conf.getTempDir() + File.separator + resultFile;
    }


    public static String getLocalVarFile(String name) {
        //seed变量信息的位置(文件名为<seed_prefix>_var.json)
        return getPrefix(getSeedFile(name)) + "_var.json";
    }

    public static String getLiveLinesFile(String name) {
        //seed live lines信息的位置(文件名为<seed_prefix>_live_line.txt)
        return getPrefix(getSeedFile(name)) + "_live.txt";
    }

    public static String getSeedPosFile(String seed, String pattern) {
        //pattern在seed上的插入位置信息 (文件名为<seed_prefix>_<pattern_prefix>_pos.txt)
        // pattern文件去掉路径和.json

        return conf.getTempDir() + File.separator + getPrefix(seed) + "_" + getPrefix(pattern) + "_pos.txt";
    }

    public static String getWASMTargetFile(String name) {
        return getPrefix(getSourceFile(name)) + ".wasm";
    }

    public static String getWasmedgeTargetFile(String name) {
        return getPrefix(getSourceFile(name)) + "_wasmedge_aot.wasm";
    }

    public static String getWamrTargetFile(String name) {
        return getPrefix(getSourceFile(name)) + "_wamr.aot";
    }

    public static class RuntimeCommand {
        private HashMap<String, String> runtimes;

        public Collection<String> getRuntimeCommands() {
            return runtimes.values();
        }

        private static Configuration conf;

        public RuntimeCommand(String testCase, String wasmTarget) {
            if (runtimes != null) {
                return;
            }
            runtimes = new HashMap<>();
            conf = Configuration.getInstance();

            String testCasePrefix = testCase.substring(0, testCase.lastIndexOf("."));
            //String[] runtimeList = {"wasmer", "wasmtime", "wasm3", "wasmedge_aot", "wamr", "wamr_aot"};
            String[] runtimeList = {"wasmer", "wasmtime", "wasmedge_aot", "wamr_aot"};
            String runCmd;
            for (String runtime : runtimeList) {
                //获取运行命令
                if (runtime.equals("wasmedge_aot")) {
                    String wasmedgeAotTarget = getWasmedgeTargetFile(testCase);
                    runCmd = conf.getWasmEdgeCommand() + " " + wasmedgeAotTarget;
                } else if (runtime.equals("wamr_aot")) {
                    String wamrAotTarget = getWamrTargetFile(testCase);
                    runCmd = conf.getWamrCommand() + " " + wamrAotTarget;
                } else if (runtime.equals("wasmtime")) {
                    runCmd = conf.getWasmTimeCommand() + " " + wasmTarget;
                } else if (runtime.equals("wasmer")) {
                    runCmd = conf.getWasmerCommand() + " run " + wasmTarget;
                } else {
                    runCmd = runtime + " " + wasmTarget;
                }
                runtimes.put(runtime, runCmd);
            }
        }
    }
}
