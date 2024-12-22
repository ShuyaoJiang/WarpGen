package executor;

import common.*;
import executor.Command;

import java.io.*;
import java.util.*;


public class Executor {
    //add by sherry
    //private static List<Double> magicList = Arrays.asList(0.30687138, 0.27069548, 0.22726855, 0.19516459);
    // 根据100个csmith seed算出来的oracle，顺序是[wasmedge, wasmtime, wasmer, wamr]
    private static List<Double> magicList = Arrays.asList(0.09610511, 0.14625995, 0.70506729, 0.05256765);
    //end
    private static Configuration conf;
    static {
        try {
            conf = Configuration.getInstance("configure.ini");
        } catch (FrameworkExceptions.ConfigurationException e) {
            throw new RuntimeException(e);
        }
    }


    public static void syncCode(String pattern, String seed, String result) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException{
        String patternFile = Command.getPatternFile(pattern);  //pattern文件的位置
        String seedFile = Command.getSeedFile(seed); //seed文件的位置
        String resultFile = Command.getCTargetFile(result); //输出文件


        //String locVarFile = Command.getLocalVarFile(seed);
        //String liveLinesFile = Command.getLiveLinesFile(seed);
        String posFile = Command.getSeedPosFile(seed, pattern);
        String header = "-- -I" + conf.getIncludePath();

        //这里调用外部命令来合成
        //String timestamp = String.valueOf(System.currentTimeMillis());
        //String outputFile = result + File.separator + seedPrefix + "_" + timestamp + ".c"; //输出文件名(<seed_prefix>_<timestamp>.c)
        String insertToolPath = conf.getInsertCommand();
        //step1: 获取插入位置 (不同pattern不一样)
        //cmd template: insert <seed_path> <block_path> <output_path> 1 <insert_pos_path> <header_dependence>
        //cmd example: insert /home/ringzzz/wasm_runtime/seed/seed.c /home/ringzzz/wasm_runtime/test-output/2mm/2mm-0.json /home/ringzzz/wasm_runtime/insertion/res.c 1 /home/ringzzz/wasm_runtime/insertion/pos.txt -- -Iinclude -I/home/ringzzz/wasm-performance-debug/csmith/install/include
        String cmd = insertToolPath + " " + seedFile + " " + patternFile + " " + resultFile + " 1 " + posFile + " " + header + " -Wno-everything";
        runCommand(cmd);
        //step2: 插入
        //cmd template: insert <seed_path> <block_path> <output_path> <insert_pos_path> <header_dependence>
        //cmd example: insert /home/ringzzz/wasm_runtime/seed/seed.c /home/ringzzz/wasm_runtime/test-output/2mm/2mm-0.json /home/ringzzz/wasm_runtime/insertion/res.c /home/ringzzz/wasm_runtime/insertion/pos.txt -- -Iinclude -I/home/ringzzz/wasm-performance-debug/csmith/install/include
        cmd = insertToolPath + " " + seedFile + " " + patternFile + " " + resultFile + " " + posFile + " " + header + " -Wno-everything"; //凑成命令行
        runCommand(cmd);

    }


    public static void runCommand (String cmd) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException{
        runCommand(cmd, null);
    }

    private static void runCommand(String cmd, String envp[]) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException{
        int status = 0;
        try {

            Log.logInfo("Invoke command: " + cmd);
            Process proc;
            String[] realCmd = {"/bin/sh", "-c", cmd};
            if(envp != null) {
                proc = Runtime.getRuntime().exec(realCmd);
            }
            else {
                proc = Runtime.getRuntime().exec(realCmd, envp);
            }
            new StreamLogger(proc.getErrorStream(), conf.getLogError()).start();
            new StreamLogger(proc.getInputStream(), conf.getLogInfo()).start();
            Worker worker = new Worker(proc);
            worker.start();
            try {
                worker.join(Command.TIME_OUT);
                if (worker.exit != null) {
                    status = proc.waitFor();
                }
                else {
                    throw new FrameworkExceptions.ExtenalCommandTimeoutException();
                }
            } catch(InterruptedException ex) {
                worker.interrupt();
                Thread.currentThread().interrupt();
                throw ex;
            } finally {
                proc.destroyForcibly();
            }
            if (status != 0) {
                throw new FrameworkExceptions.ExtenalCommandAbortException(status);
            }
        }
        catch (InterruptedException | IOException | FrameworkExceptions.ExtenalCommandAbortException | FrameworkExceptions.ExtenalCommandTimeoutException e ) {
            Log.logError("Command " + cmd + " called failure!");
            if(e instanceof FrameworkExceptions.ExtenalCommandAbortException) {
                if(status == 100) {
                    //this means the error is not fatal, may recover via a retry
                    Log.logInfo("May encounter non-fatal error.");
                }
                else {
                    Log.logInfo("Encounter fatal error.");
                }
                throw (FrameworkExceptions.ExtenalCommandAbortException)e;
            }
            else if (e instanceof FrameworkExceptions.ExtenalCommandTimeoutException) {
                Log.logInfo("Encounter time-out error.");
                throw (FrameworkExceptions.ExtenalCommandTimeoutException)e;
            }
            else {
                Log.logError("System exception, probably due to bug.");
                System.exit(1);
            }
        }
        Log.logInfo("Command invoked successfully.");
    }


    private static class Worker extends Thread {
        private final Process process;
        private Integer exit;
        private Worker(Process process) {
            this.process = process;
        }
        public void run() {
            try {
                exit = process.waitFor();
            } catch (InterruptedException ignore) {
                return;
            }
        }
    }


    public static void main (String[] args) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException {
        compile("seed0.c");

        syncCode("2mm-1.json", "seed0.c", "111.c");
        Log.logInfo("Resulting Distinguishability = " + runCase("111.c"));
        splitBadCase("111.c");
        //splitBadCase("2mm.c"); //这个需要2mm.c所在目录下有include文件夹(已存在，但换目录的话需要拷贝)
    }



    public static void compile(String testCase) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException {
        Log.logInfo("Starting to compile " + testCase);
        String source = Command.getSourceFile(testCase);
        String wasmTarget = Command.getWASMTargetFile(testCase);
        String emccPath = conf.getEmccCommand();
        String headerPath = conf.getIncludePath();
        String patternHeaderPath = conf.getPatternIncludePath();
        //cmd template: emcc -O2 -s WASM=1 -s TOTAL_MEMORY=512MB -I <header> -I <pattern_header> <source> -o <wasm_target>
        //cmd example: emcc -O2 -s WASM=1 -s TOTAL_MEMORY=512MB -I /home/syjiang21/csmith/include -I /data/project/syjiang/llvm-project/build/warp-test/source/include /data/project/syjiang/llvm-project/build/insert-test/seed/random1.c -o /data/project/syjiang/llvm-project/build/insert-test/seed/random1.wasm
        String compileCmd = emccPath + " -O2 -s WASM=1 -s TOTAL_MEMORY=512MB -I" + headerPath + " -I" + patternHeaderPath + " " + source + " -o " + wasmTarget;
        runCommand(compileCmd);

        //编译wasmedge和wamr的target
        String wasmedgeAotTarget = Command.getWasmedgeTargetFile(testCase);
        String wasmedgeCompileCmd = conf.getWasmEdgeCCommand() + " " + wasmTarget + " " + wasmedgeAotTarget;
        runCommand(wasmedgeCompileCmd);

        String wamrAotTarget = Command.getWamrTargetFile(testCase);
        String wamrCompileCmd = conf.getWamrCCommand() + "  -o " + wamrAotTarget + " " + wasmTarget;
        runCommand(wamrCompileCmd);
        Log.logInfo(testCase + " compiled, OK!");
    }

    public static double runCase(String testCase) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException{


        //e在每个runtime上运行wasm并记录时间，写进一个list(time vector)
        String wasmTarget = Command.getWASMTargetFile(testCase);
        List<Long> execTimeList = new ArrayList<>();
        for (String runCmd: new Command.RuntimeCommand(testCase, wasmTarget).getRuntimeCommands()) {

            long startTime = System.currentTimeMillis();
            runCommand(runCmd);
            long endTime = System.currentTimeMillis();
            long execTime = endTime - startTime;
            execTimeList.add(execTime);
        }

        String timeVector = testCase + ",";
        long timeAll = 0;
        String text = "Resulting time vector: [";
        for(Long time: execTimeList) {
            timeVector += time + ",";
            text += time + " ";
            timeAll += time;
        }
        text += "]";
        //time vector normalization and save into csv
        Log.logInfo(text);
        List<Double> normalizedExecTimeList = new ArrayList<>();
        for(Long time: execTimeList) {
            timeVector += (double)(time)/timeAll + ",";
            normalizedExecTimeList.add((double)(time)/timeAll);
        }
        //Step4: 计算 distinguishability
        double distinguishability = calDistance(normalizedExecTimeList);
        timeVector += distinguishability + ",";
        try {
            Log.log(conf.getLogCSVFile(), timeVector);
        }
        catch (IOException e) {
            Log.logError("Cannot access CSV File " + conf.getLogCSVFile());
            System.exit(1);
        }
        return distinguishability;
    }

    private static double calDistance(List<Double> normalizedExecTimeList) {

        if(normalizedExecTimeList.size() != magicList.size()) {
            Log.logError("Fatal error in distinguishability settings: size mismatch!");
            System.exit(1);
        }
        double result = 0.0;
        String text = "Resulting vector: ";
        for(int i = 0; i < normalizedExecTimeList.size(); i++) {
            double dis = magicList.get(i) - normalizedExecTimeList.get(i);
            dis = dis * dis;
            result += dis;
            text += normalizedExecTimeList.get(i) + " ";
        }
        Log.logInfo(text);
        return Math.sqrt(result);
    }

    public static void splitBadCase(String badCase) throws FrameworkExceptions.ExtenalCommandAbortException, FrameworkExceptions.ExtenalCommandTimeoutException{

        badCase = conf.getTempDir() + File.separator + badCase; //bad case的位置
        String outputPath = conf.getPathPattern(); //输出到pattern pool中
        String spliterPath = conf.getSplitter(); //spliter(extract-blocks)的位置
        String header = "-- -I" + conf.getIncludePath() + " -I" + conf.getPatternIncludePath();
        //cmd template: <tool_path> <source_path> <output_path> -- -I<header_path_1> -I<header_path_2>...
        //cmd example: /data/project/syjiang/llvm-project/build/bin/extract-blocks /data/project/syjiang/llvm-project/build/warp-test/source/2mm.c /data/project/syjiang/llvm-project/build/warp-test/pattern-pool -- -I/data/project/syjiang/csmith/include
        String cmd =  spliterPath + " " + badCase + " " + outputPath + " " + header;
        runCommand(cmd);

    }

}
