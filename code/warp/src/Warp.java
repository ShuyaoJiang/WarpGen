import common.Configuration;
import common.FrameworkExceptions;
import common.Log;
import executor.Command;
import executor.Executor;
import filemanager.*;

import java.io.*;
import java.util.Iterator;
import java.util.Map.Entry;


public class Warp {

    private static final int MAX_RETRY_IN_INITIAL = 8;
    private static final int MAX_RETRY_IN_FUZZ = 2;

    private static final int MIN_RETRY_FOR170 = 4; //从未成功的，试四次插入不成功就扔了
    private static final int MAX_RETRY_FOR170 = 8; //成功插过的，试八次插入不成功就扔了

    private static final int  MAX_FAILURE_FOR_SUCCESSFUL_170 = 5;

    private SourceFileManager patternManager;
    private  SourceFileManager seedManager;
    private TargetFileManager targetManager;
    private SourceFileProbe patternProbe;
    private  Configuration conf;
    private  int patternNum = 0;
    private  int seedNum = 0;

    private  int numTry = 0;
    private  int numDone = 0;

    public static void main(String[] args) {
        if(args.length != 1) {
            Log.logError("parameter: all or fuzz");
        }
        else if(args[0].compareToIgnoreCase("all")==0) {
            new Warp().start();
        }
        else if(args[0].compareToIgnoreCase("fuzz")==0) {
            Warp warp = new Warp();
            warp.loadFiles();
            new Warp().startFuzz();
        }
        else {
            Log.logError("parameter: all or fuzz");
        }
    }
    Warp() {
        try {
            conf = Configuration.getInstance("configure.ini");
            patternManager = new SourceFileManager();
            seedManager = new SourceFileManager();
            targetManager = new TargetFileManager(Configuration.getInstance().getTopN());
            patternProbe = new SourceFileProbe(patternManager, conf.getPathPattern(), conf.getMagicStringPattern());

            SourceFileProbe seedProbe = new SourceFileProbe(seedManager, conf.getPathSeed(), conf.getMagicStringSeed());
            Log.logCriticalInfo("Start to probe patterns and seeds.");
            patternProbe.doOnce();//startInNewThread();
            patternNum = patternManager.getAlive();
            Log.logInfo(patternNum + " patterns found.");
            seedProbe.doOnce();//startInNewThread();
            seedNum = seedManager.getAlive();
            Log.logInfo(seedNum + " seeds found.");
            Log.logCriticalInfo("Done: probing patterns and seeds.");
        }
        catch (FrameworkExceptions.ConfigurationException e) {
            Log.logError("Fatal Error: Configuration exception.");
            System.exit(1);
        }

        //start to process all the patterns to obtain an initial top-n distinguishability values
        if(patternNum == 0 || seedNum == 0) {
            Log.logError("No pattern or no seed found.");
            System.exit(1);
        }
    }
    public String getStatus() {
        return " (Try=" + numTry + ", Done=" + numDone + ", Alived=" + patternManager.getAlive() + ", All=" + patternManager.getSize() + ").";
    }
    public void start() {

        //先把所有pattern跑一下
        runWithAllIntialPatterns();
        //split前n个
        splitIntialTopN();

        saveFiles(".fuzz");

        startFuzz();

    }

    private void saveFile(String fileName, Object obj) throws IOException {
        FileOutputStream fos = new FileOutputStream(fileName);
        ObjectOutputStream oos = new ObjectOutputStream(fos);
        oos.writeObject(obj);
    }

    private  Object loadFile(String fileName) throws IOException, ClassNotFoundException {
        FileInputStream fis = new FileInputStream(fileName);
        ObjectInputStream ois = new ObjectInputStream(fis);
        return ois.readObject();
    }

    private void saveFiles() {
        saveFiles("");
    }

    private void saveFiles(String fileNameExt) {
        Log.logCriticalInfo("Start to save file records.");
        try {
            saveFile(Command.getPatternManagerFile()+fileNameExt, patternManager);
            saveFile(Command.getSeedManagerFile()+fileNameExt, seedManager);
            saveFile(Command.getResultManagerFile()+fileNameExt, targetManager);
        } catch (IOException e) {
            Log.logError("Cannot save data, please check the conf of SEED_FILE, PATTERN_FILE, and RESULT_FILE");
            System.exit(1);
        }
        Log.logCriticalInfo("Done: File records saved.");
    }

    private void loadFiles() {
        loadFiles("");
    }
    private void loadFiles(String fileNameExt) {
        Log.logCriticalInfo("Start to load file records.");
        try {
            patternManager = (SourceFileManager) loadFile(Command.getPatternManagerFile()+fileNameExt);
            seedManager = (SourceFileManager) loadFile(Command.getSeedManagerFile()+fileNameExt);
            targetManager = (TargetFileManager) loadFile(Command.getResultManagerFile()+fileNameExt);
        } catch (IOException | ClassNotFoundException | ClassCastException e) {
            Log.logError("Cannot load data, please check the conf of SEED_FILE, PATTERN_FILE, and RESULT_FILE");
            System.exit(1);
        }
        try {
            conf.loadConf();
        }
        catch (FrameworkExceptions.ConfigurationException e) {
            Log.logError("Fatal Error: Configuration exception.");
            System.exit(1);
        }
        patternProbe = new SourceFileProbe(patternManager, conf.getPathPattern(), conf.getMagicStringPattern());

        Log.logCriticalInfo("Done: File records loaded.");
    }
    private void startFuzz() {
        Log.logCriticalInfo("Start to perform fuzz test.");
        boolean shouldChange = true; //让下面循环一开始是要产生新pattern的
        String patternFileName = "";

        for(int i = 0; ; i++) {
            if(shouldChange) {
                //从pattern取出一个pattern
                try {
                    for (; ; ) {
                        //patternFileName = patternManager.getRandomFile(conf.getMaxPenalty());
                        patternFileName = patternManager.getMaxDistinguishabilityFile(conf.getMaxPenalty());
                        File file = new File(Command.getPatternFile(patternFileName));
                        if (file.exists()) {
                            (patternManager.getFile(patternFileName)).setDead();
                            break;
                        } else {
                            Log.logError("Dead file: " + patternFileName);
                        }
                    }
                } catch (FrameworkExceptions.TargetNotFoundException e) {
                    break;
                }
            }
            else {
                //这个block只为注释用：说明这个pattern之前顺利跑起了，我们要物尽其用
            }
            shouldChange = true;
            SourceFile patternFile = (SourceFile) patternManager.getFile(patternFileName);
            patternFile.inc();
            //拼接跑跑看
            String resultFileName;
            try {
                resultFileName = runWithARandomSeed(patternFileName, MAX_RETRY_IN_FUZZ);
            }
            catch (FrameworkExceptions.TargetNotFoundException ex) {
                Log.logError("Cannot find available seed for pattern: " + patternFileName);
                //两种可能
                //1. 未成功过，或是成功过但一直不能成功的170，那删除
                if(!patternManager.isSuccessful(patternFile.name) ||
                        (patternManager.isSuccessful(patternFile.name) && patternManager.getFailureCount(patternFile.name) >= MAX_FAILURE_FOR_SUCCESSFUL_170)) {
                    try {
                        patternManager.finalizeFile(patternFile.name);
                    } catch (FrameworkExceptions.TargetNotFoundException e) {
                        Log.logError("Bug found ##6, please report!");
                        System.exit(1);
                    }
                    //真的让他dead，文件名字变为.json.dead
                    File file = new File(Command.getPatternFile(patternFileName));
                    Log.logError("Remove pattern file (cannot insert or emcc): " + (file.renameTo(new File(Command.getPatternFile(patternFileName) + ".dead")) ? "Success" : "Failure"));
                }
                //2. 否则，那我们不删，但是让170 pattern失败计数++
                if(patternFile.name.startsWith("170")) {
                    patternManager.incFailureCount(patternFile.name);
                }
                //换pattern，啥也不干
                Log.logInfo("Will not delete pattern because it has been a successful pattern: " + patternFileName);
                continue;
            }
            TargetFile targetFile = (TargetFile) targetManager.getFile(resultFileName);
            SourceFile sourceFile = (SourceFile) patternManager.getFile(patternFileName);
            if (!targetFile.isDead()) {
                //和top n比较，替代了top n了，说明这是有效的
                //pattern的penalty清零
                sourceFile.reset();
                //split自己，放到pattern里
                try {
                    Executor.splitBadCase(resultFileName);
                    patternProbe.doOnce(targetFile.distinguishability); //因为分离成功了，所以要重新probe
                } catch (FrameworkExceptions.ExtenalCommandAbortException |
                         FrameworkExceptions.ExtenalCommandTimeoutException e) {
                    Log.logError("Bad case split error, skip it!");
                }
                //跑到这里，说明这个pattern能成功替代，我们继续用这个pattern
                shouldChange = false;
            }
            else {
                //无效的
                sourceFile.punish();
                if (sourceFile.getPenalty() > conf.getMaxPenalty()) {
                    // 判断pattern的penalty是不是大于阈值
                    // 如果是，pattern setDead, 以后不用这个pattern
                    try {
                        patternManager.finalizeFile(sourceFile.name);
                    } catch (FrameworkExceptions.TargetNotFoundException e) {
                        Log.logError("Bug found ##5, please report!");
                        System.exit(1);
                    }
                    //真的让他dead，文件名字变为.json.dead
                    File file = new File(Command.getPatternFile(patternFileName));
                    Log.logError("Remove pattern file (penalty): " + (file.renameTo(new File(Command.getPatternFile(patternFileName) + ".dead"))? "Success": "Failure"));
                }
            }
            saveFiles();
        }
        Log.logCriticalInfo("Done: fuzz test.");
    }


    private void __test() {
    /*

    初始轮runWithARandomSeed（each 原始pattern)，maxRetry设为MAX_RETRY_IN_INITIAL（8）
    fuzz轮
    --循环{
    ----runWithARandomSeed（each of all alived pattern)，maxRetry设为MAX_RETRY_IN_FUZZ (2）
    --------如果不成功
    ------------如果不是【执行成功过的pattern】，也就是这是一个没成功过的170 pattern，删了pattern，换个pattern，继续循环
    ------------否则，也就是这是一个【执行成功过的pattern】
    ----------------如果是原始pattern，那不删，换个pattern，继续循环
    ----------------如果是是170pattern，那它这么不成功5次（5*2，也就是10次插入失败）就删了，换个pattern，继续循环
    --------如果成功，看是否替代了top n
    ------------如果是，替代，拆开做新pattern，将pattern惩罚清零，继续循环
    ------------如果不是，惩罚+1
    ----------------如果惩罚超过阈值（配置项的MAX_PENALTY：5），，删了pattern，换个pattern，继续循环
    ----------------否则，不删，换个pattern，继续循环
    --}




    runWithARandomSeed: 输入pattern
       循环{
            对于【执行成功过的pattern】： 如果失败次数（循环了)maxRetry次，退出循环（这只会在fuzz轮，因为初始轮每个pattern跑一次，不可能遇到曾经执行成功的pattern）
            否则，对于不是【执行成功过的原始pattern】，如果失败次数（循环了)maxRetry次，退出循环（这只会在初始轮进行，因为fuzz轮不会有不执行成功过的原始pattern）
                 对于不是【执行成功过的170 pattern】，如果是【插入成功过的pattern】，如果失败次数（循环了)MAX_RETRY_FOR170(8)，退出循环
                                              如果不是【插入成功过的pattern】，如果失败次数（循环了)MIN_RETRY_FOR170(4)，退出循环
            取个随机种子
            合成
                如果合成不成功，失败次数++，标记这个seed不可选，然后继续循环，跳过后面步骤
            如果是170 pattern，标记为【插入成功过的pattern】
            编译、执行
                如果不成功，失败次数++，标记这个seed不可选，然后继续循环，跳过后面步骤
            存结果
            设为曾经执行成功的pattern
            重置所有seed以后可选
       }







       }


    */}

    private String runWithARandomSeed(String patternFileName, int maxRetry) throws FrameworkExceptions.TargetNotFoundException{
        String resultFileName;
        int syncFailure = 0;
        numTry ++;
        Log.logCriticalInfo("Start to run with pattern: " + patternFileName + getStatus());
        for(;;) {
            if(syncFailure == maxRetry && patternManager.isSuccessful(patternFileName)) {
                //fuzz轮的成功过的pattern
                seedManager.resetPenalty();
                Log.logCriticalInfo("Fail to run with pattern: " + patternFileName);
                throw new FrameworkExceptions.TargetNotFoundException();
            }
            else if(syncFailure == maxRetry && !patternFileName.startsWith("170")) {
                //初始轮的pattern
                seedManager.resetPenalty();
                Log.logCriticalInfo("Fail to run with pattern: " + patternFileName);
                throw new FrameworkExceptions.TargetNotFoundException();
            }
            else if(patternFileName.startsWith("170")) {
                //fuzz轮的未成功过的170 pattern
                if(patternManager.isGood170(patternFileName)) {
                    if (syncFailure == MAX_RETRY_FOR170) {
                        seedManager.resetPenalty();
                        Log.logCriticalInfo("Fail to run with pattern: " + patternFileName);
                        throw new FrameworkExceptions.TargetNotFoundException();
                    }
                }
                else {
                    if (syncFailure == MIN_RETRY_FOR170) {
                        seedManager.resetPenalty();
                        Log.logCriticalInfo("Fail to run with pattern: " + patternFileName);
                        throw new FrameworkExceptions.TargetNotFoundException();
                    }
                }
            }
            String seedFileName;
            try {
                //get a random seed, loop can break in this line because of exception
                seedFileName = seedManager.getRandomFile(0);
            }
            catch (FrameworkExceptions.TargetNotFoundException e) {
                seedManager.resetPenalty();
                Log.logCriticalInfo("Fail to run with pattern: " + patternFileName);
                throw e;
            } 
            double distinguishability = 0;
            resultFileName = System.currentTimeMillis() + ".c";
            //call the syn command, save as the resultFileName file in the temp directory (String tempDir)
            try {
                Executor.syncCode(patternFileName, seedFileName, resultFileName);
            }
            catch (FrameworkExceptions.ExtenalCommandAbortException | FrameworkExceptions.ExtenalCommandTimeoutException e) {
                if((e instanceof FrameworkExceptions.ExtenalCommandAbortException && ((FrameworkExceptions.ExtenalCommandAbortException)e).getCode() == 100)
                        || (e instanceof FrameworkExceptions.ExtenalCommandTimeoutException)){
                    //non-fatal error
                    //have to find another seed
                    ((SourceFile)seedManager.getFile(seedFileName)).punish();
                    syncFailure ++;
                    continue;
                }
            }
            if(patternFileName.startsWith("170") && !patternManager.isGood170(patternFileName)) {
                Log.logInfo("We are happy to find a new good 170 pattern");
                patternManager.setGood170(patternFileName);
                syncFailure = 0;//重新计算失败
            }
            //run the case and get the distinguishability value
            try {
                //if error found, we skip this seed and continue
                Executor.compile(resultFileName);
                distinguishability = Executor.runCase(resultFileName);
            }
            catch (FrameworkExceptions.ExtenalCommandAbortException | FrameworkExceptions.ExtenalCommandTimeoutException e) {
                ((SourceFile)seedManager.getFile(seedFileName)).punish();
                syncFailure ++;
                continue;
            }
            finally {
                try {
                    Executor.runCommand("killall wasmedge");
                }
                catch (FrameworkExceptions.ExtenalCommandAbortException | FrameworkExceptions.ExtenalCommandTimeoutException e) {
                    //do nothing
                }
            }
            //save the result
            if (!targetManager.addFile(new TargetFile(resultFileName, patternFileName, seedFileName, distinguishability))) {
                Log.logError("Bug found ##1, please report!");
                System.exit(1);
            }
            try {
                Log.log(conf.getLogCSVFile(), patternFileName + "," + seedFileName + System.getProperty("line.separator"));
            }
            catch (IOException e) {
                Log.logError("Cannot access CSV File " + conf.getLogCSVFile());
                System.exit(1);
            }
            patternManager.setSuccessful(patternFileName);
            if(distinguishability > patternManager.getMaxDistinguishability(patternFileName)) {
                Log.logInfo("Update distin to:  " + distinguishability + "(pattern = " + patternFileName + ")");
                patternManager.setMaxDistinguishability(patternFileName, distinguishability);
            }
            seedManager.resetPenalty();
            Log.logCriticalInfo("Done: run with pattern: " + patternFileName);
            numDone++;
            return resultFileName;
        }
    }

    private void runWithAllIntialPatterns() {
        Log.logCriticalInfo("Start to run with initial patterns.");
        Iterator<Entry<String, WarpFile>> iterator = patternManager.getFileIterator();
        while (iterator.hasNext()) {
            //for each pattern
            Entry<String, WarpFile> entry = iterator.next();
            String patternFileName = entry.getKey();
            SourceFile patternFile = (SourceFile)entry.getValue();
            patternFile.inc();
            try {
                runWithARandomSeed(patternFileName, MAX_RETRY_IN_INITIAL);
            }
            catch (FrameworkExceptions.TargetNotFoundException ex) {
                Log.logError("Cannot find available seed for pattern: " + patternFileName);
                try {
                    patternManager.finalizeFile(patternFile.name);
                } catch (FrameworkExceptions.TargetNotFoundException e) {
                    Log.logError("Bug found ##5, please report!");
                    System.exit(1);
                }
                //真的让他dead，文件名字变为.json.dead
                File file = new File(Command.getPatternFile(patternFileName));
                Log.logError("Remove pattern file: " + (file.renameTo(new File(Command.getPatternFile(patternFileName) + ".dead"))? "Success": "Failure"));
            }
            //让pattern的初始惩罚为-3
            //patternFile.punish();
        }
        Log.logCriticalInfo("Done: Initial patterns inserted and executed.");
    }

    private void splitIntialTopN() {
        Log.logCriticalInfo("Start to split the top-n slow cases.");
        //top n个case 就是 targetManager里面那些live （not dead) 的
        Iterator<Entry<String, WarpFile>> iterator = targetManager.getFileIterator();
        while (iterator.hasNext()) {
            //for slow case
            Entry<String, WarpFile> entry = iterator.next();
            String targetFileName = entry.getKey();
            TargetFile targetFile = (TargetFile) entry.getValue();
            if (targetFile.isDead()) {
                continue;
            }
            //for each slow case
            //split他们，放到pattern里
            try {
                Log.logInfo("Obtaining patterns from slow case: " + targetFileName);
                Executor.splitBadCase(targetFileName);
                //扫描新增，并加入，新增的distin就是上次结果的distin
                patternProbe.doOnce(targetFile.distinguishability);
            }
            catch (FrameworkExceptions.ExtenalCommandAbortException | FrameworkExceptions.ExtenalCommandTimeoutException e) {
                Log.logError("Bad case split error, skip it!");
            }

        }
        Log.logCriticalInfo("Done: Split the top-n slow cases.");
    }

}