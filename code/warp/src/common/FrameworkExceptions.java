package common;

public class FrameworkExceptions {
    public static class TargetNotFoundException extends Exception {
    }
    public static class ConfigurationException extends Exception {
    }
    public static class ExtenalCommandAbortException extends Exception {
        int code;
        public ExtenalCommandAbortException(int code) {
            this.code = code;
        }
        public int getCode() {
            return code;
        }
    }
    public static class ExtenalCommandTimeoutException extends Exception {
    }
}
