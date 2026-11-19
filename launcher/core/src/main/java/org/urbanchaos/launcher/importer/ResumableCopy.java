package org.urbanchaos.launcher.importer;
import java.io.*;
import java.util.Arrays;
/** Copies an exact-size ISO to private staging; verifies every persisted prefix byte before resume. */
public final class ResumableCopy {
    public interface State { boolean cancelled(); void progress(long bytes,long total,boolean verifying); }
    public static final class Paused extends IOException {public Paused(){super("UC-IMPORT-PAUSED");}}
    private static int read(InputStream input,byte[] bytes,int count)throws IOException {
        int done=0;while(done<count){int n=input.read(bytes,done,count-done);if(n<0)throw new EOFException("UC-IMPORT-TRUNCATED");if(n==0)throw new IOException("UC-IMPORT-NO-PROGRESS");done+=n;}return done;
    }
    public static void copy(InputStream input,File partial,long total,long free,State state)throws IOException {
        if(total<=0||total>16L*1024*1024*1024)throw new IOException("UC-IMPORT-SIZE");
        long done=partial.exists()?partial.length():0;
        if(done>total)throw new IOException("UC-IMPORT-RESUME-SIZE");
        if(free<total-done+64L*1024*1024)throw new IOException("UC-IMPORT-SPACE");
        byte[] source=new byte[1024*1024],saved=new byte[source.length];
        if(done>0)try(InputStream previous=new FileInputStream(partial)){
            long checked=0;
            while(checked<done){if(state.cancelled())throw new Paused();int n=(int)Math.min(source.length,done-checked);read(input,source,n);read(previous,saved,n);
                if(!Arrays.equals(source,saved))throw new IOException("UC-IMPORT-RESUME-MISMATCH");
                checked+=n;state.progress(checked,total,true);
            }
        }
        try(FileOutputStream output=new FileOutputStream(partial,true)){
            long synced=done;
            while(done<total){if(state.cancelled())throw new Paused();int n=(int)Math.min(source.length,total-done);read(input,source,n);output.write(source,0,n);done+=n;
                if(done-synced>=16L*1024*1024){output.getFD().sync();synced=done;}state.progress(done,total,false);
            }
            if(state.cancelled())throw new Paused();
            if(input.read()!=-1)throw new IOException("UC-IMPORT-SOURCE-CHANGED");output.getFD().sync();
        }
    }
}
