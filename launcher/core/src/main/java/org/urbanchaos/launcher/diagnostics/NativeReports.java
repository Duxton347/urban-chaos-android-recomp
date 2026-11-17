package org.urbanchaos.launcher.diagnostics;

import java.io.IOException;
import static org.urbanchaos.launcher.diagnostics.ReportSession.detail;

/** Compile the JNI source into the same library as the game's SDK logger (normally libmain.so). */
public final class NativeReports {
    private NativeReports(){}
    // The caller loads the real game library. Do not load a second SDK logger in another library.
    public static synchronized boolean attach(ReportSession session,String build,String xbeHash,boolean detailed){
        try {
            boolean ready=start(session.nativeDirectory().getCanonicalPath(),build,xbeHash,detailed);
            if(!ready)session.event(ReportSession.Event.NATIVE_LOG_FAILURE,detail("reason","native_start_failed"));
            return ready;
        }catch(IOException error){
            session.event(ReportSession.Event.NATIVE_LOG_FAILURE,detail("reason","native_directory_unavailable"));
            return false;
        }catch(LinkageError error){
            session.event(ReportSession.Event.NATIVE_LOG_FAILURE,detail("reason","native_library_unavailable"));
            return false;
        }
    }
    public static synchronized boolean healthy(){
        try{return isHealthy();}catch(LinkageError error){return false;}
    }
    /** Stop only after the previous guest threads have stopped, before closing the Java session. */
    public static synchronized void detach(){
        try{stop();}catch(LinkageError ignored){}
    }
    private static native boolean start(String directory,String build,String xbeHash,boolean detailed);
    private static native boolean isHealthy();
    private static native void stop();
}
