package org.urbanchaos.launcher.diagnostics;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import java.io.*;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.Executor;
import static org.urbanchaos.launcher.diagnostics.ReportSession.detail;

/** Integration adapter for the real launcher, not an Activity or a simulated game. */
public final class AndroidReports {
    private final Context context;
    private final ReportSession session;
    public interface Result { void completed(boolean success,String code); }

    public AndroidReports(Context owner,String build,String xbeHash) throws IOException {
        context=owner.getApplicationContext();
        session=new ReportSession(new File(context.getFilesDir(),"logs/reports"),build,xbeHash);
        session.installUncaughtHandler();
        session.event(ReportSession.Event.DEVICE,detail("api",Build.VERSION.SDK_INT,
            "model",ReportSession.deviceToken(Build.MODEL),"abi",ReportSession.deviceToken(Build.SUPPORTED_ABIS[0])));
        if(Build.VERSION.SDK_INT>=30)recordPreviousExit();
    }
    public ReportSession session(){return session;}
    private void recordPreviousExit(){
        try {
            ActivityManager manager=(ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
            if(manager==null)return;
            List<ApplicationExitInfo> exits=manager.getHistoricalProcessExitReasons(context.getPackageName(),0,1);
            if(!exits.isEmpty()){
                ApplicationExitInfo exit=exits.get(0);
                session.event(ReportSession.Event.NATIVE_EXIT,detail("reason",exit.getReason(),
                    "status",exit.getStatus(),"pss_kb",exit.getPss(),"rss_kb",exit.getRss(),"exit_utc_ms",exit.getTimestamp()));
            }
        }catch(SecurityException ignored){
            // Absence of OS exit metadata must not prevent launching the original game.
        }
    }
    /** Use the launcher's Activity Result API; a null result is cancellation, not a failure. */
    public Intent createExportDocument(){
        return new Intent(Intent.ACTION_CREATE_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE)
            .setType("application/zip").putExtra(Intent.EXTRA_TITLE,"urban-chaos-report.zip");
    }
    /** work must be a background executor; ui must post to the Android UI thread. */
    public void exportTo(Uri destination,Executor work,Executor ui,Result result){
        if(destination==null){ui.execute(()->result.completed(false,"UC-EXPORT-CANCELLED"));return;}
        work.execute(()->{
            File temporary=null;String code="UC-EXPORT-002";boolean success=false,opened=false;
            try {
                temporary=File.createTempFile("urban-report-",".zip",context.getCacheDir());
                try(OutputStream out=new FileOutputStream(temporary)){session.exportZip(out);}
                try(InputStream input=new FileInputStream(temporary);
                    OutputStream output=context.getContentResolver().openOutputStream(destination,"wt")){
                    if(output==null)throw new IOException("No destination stream");
                    opened=true;
                    byte[] buffer=new byte[65536];int count;
                    while((count=input.read(buffer))!=-1)output.write(buffer,0,count);
                    output.flush();
                }
                success=true;code="UC-EXPORT-000";
            }catch(IOException|SecurityException error){
                // Never put provider URI, exception message or user path into the exported report.
                if(opened)try{context.getContentResolver().delete(destination,null,null);}catch(Exception ignored){}
            }finally{
                if(temporary!=null)temporary.delete();
            }
            boolean done=success;String finalCode=code;
            ui.execute(()->result.completed(done,finalCode));
        });
    }
    public void lifecycle(String state){
        session.event(ReportSession.Event.LIFECYCLE,detail("state",state));
    }
    // onPause/onStop are not normal session termination: Android may resume the same process.
    public void closeNormally() throws IOException {session.close();}
}
