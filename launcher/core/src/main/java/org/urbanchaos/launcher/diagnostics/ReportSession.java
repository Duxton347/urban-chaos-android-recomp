package org.urbanchaos.launcher.diagnostics;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.zip.*;

/** Structured, bounded diagnostics. Never accepts exception messages, game data or arbitrary text. */
public final class ReportSession implements AutoCloseable {
    public enum Event {
        START("UC-APP-001","info","Launcher session started",true),
        END("UC-APP-002","info","Launcher session ended normally",true),
        PREVIOUS_UNCLEAN("UC-APP-003","warn","Previous session has no clean end; crash, OS kill or power loss are possible",false),
        LIFECYCLE("UC-APP-004","info","Application lifecycle changed",true),
        JAVA_FAILURE("UC-JAVA-001","error","Uncaught Java exception; root cause requires analysis",false),
        JAVA_FRAME("UC-JAVA-002","error","Java call stack location; argument values omitted",false),
        IMPORT_START("UC-IMPORT-001","info","Disc import started",true),
        IMPORT_PROGRESS("UC-IMPORT-002","info","Disc import progress",true),
        IMPORT_READY("UC-IMPORT-003","info","Validated import activated",true),
        IMPORT_CANCELLED("UC-IMPORT-004","info","Disc import cancelled",true),
        IMPORT_FAILED("UC-IMPORT-005","error","Import failed; root cause not classified",false),
        LOCALE_PREPARED("UC-LOCALE-004","info","Optional native resource cache prepared; gameplay and subtitle synchronization not tested",true),
        LOCALE_STATE("UC-LOCALE-005","info","Optional native archive selection at game launch; no paths or caption data",true),
        LOCALE_FAILED("UC-LOCALE-001","error","Optional resource preparation failed; source revision or IO needs inspection",false),
        DEVICE("UC-DEVICE-001","info","Device capability metadata",true),
        NATIVE_EXIT("UC-NATIVE-001","warn","Android reported a previous process exit; inspect OS reason code",false),
        INPUT_PROFILE("UC-INPUT-001","info","Input profile changed; individual touches are omitted",true),
        SAVE_RESULT("UC-SAVE-001","info","Save operation status; save contents omitted",false),
        NATIVE_LOG_FAILURE("UC-LOG-005","error","Native collector unavailable; inspect reason token",false),
        EXPORT_SUCCESS("UC-EXPORT-000","info","Report was written to the selected destination",true),
        EXPORT_FAILURE("UC-EXPORT-002","error","Report export failed; underlying cause may require analysis",false),
        EXPORT_PICKER_UNAVAILABLE("UC-EXPORT-003","error","Document picker is unavailable or access was refused",false),
        EXPORT_PACKED("UC-EXPORT-005","info","Diagnostic ZIP prepared; destination delivery not yet confirmed",true),
        EXPORT_CANCELLED("UC-EXPORT-CANCELLED","info","Report export was cancelled",true),
        EXPORT("UC-EXPORT-001","info","Diagnostic export requested",true);
        final String code, level, reason; final boolean known;
        Event(String c,String l,String r,boolean k){code=c;level=l;reason=r;known=k;}
    }
    public static final int DEFAULT_LIMIT=256*1024;
    private static final int MAX_FILE=1024*1024, MAX_SESSIONS=3;
    private static final Set<String> KEYS=new HashSet<>(Arrays.asList("state","bytes","total","success","api","model","abi",
        "gpu","driver","exception_class","class","method","line","reason","status","pss_kb","rss_kb",
        "profile","duration_ms","previous_count","counter","mode","exit_utc_ms","xbe_sha256"));
    private final Path root, directory;
    private final int limit;
    private final long started=System.nanoTime();
    private long sequence;
    private OutputStream output;
    private int size;
    private boolean healthy=true, closed=false;
    private String failureCode="";
    private Thread.UncaughtExceptionHandler installed, previous;

    public ReportSession(File reportsRoot,String build,String xbeHash) throws IOException {
        this(reportsRoot,build,xbeHash,DEFAULT_LIMIT);
    }
    public ReportSession(File reportsRoot,String build,String xbeHash,int bytesPerFile) throws IOException {
        if(!token(build)||!(xbeHash.equals("unselected")||xbeHash.matches("[0-9a-f]{64}"))
                ||bytesPerFile<4096||bytesPerFile>DEFAULT_LIMIT)throw new IllegalArgumentException("Invalid diagnostic configuration");
        root=reportsRoot.toPath().toAbsolutePath().normalize();limit=bytesPerFile;
        if(Files.isSymbolicLink(root))throw new IOException("UC-LOG-002");
        Files.createDirectories(root);
        int unclean=0;
        for(Path path:sessions())if(Files.exists(path.resolve("session.open"),LinkOption.NOFOLLOW_LINKS)||Files.exists(path.resolve("native/session.open"),LinkOption.NOFOLLOW_LINKS))unclean++;
        directory=root.resolve(UUID.randomUUID().toString());Files.createDirectory(directory);
        String manifest="{\"schema\":1,\"build\":"+quote(build)+",\"xbe_sha256\":"+quote(xbeHash)+
            ",\"sdk\":\"c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7\"}\n";
        Files.write(directory.resolve("manifest.json"),manifest.getBytes(StandardCharsets.UTF_8),StandardOpenOption.CREATE_NEW);
        Files.createFile(directory.resolve("session.open"));
        output=Files.newOutputStream(directory.resolve("launcher.0.jsonl"),StandardOpenOption.CREATE_NEW);
        event(Event.START,Collections.emptyMap());
        if(unclean>0)event(Event.PREVIOUS_UNCLEAN,detail("previous_count",unclean));
        retain();
    }
    public File directory(){return directory.toFile();}
    /** Pass this fresh child path to the native journal; do not store game resources here. */
    public File nativeDirectory(){return directory.resolve("native").toFile();}
    public synchronized boolean healthy(){return healthy;}
    public synchronized String failureCode(){return failureCode;}

    public static Map<String,Object> detail(Object... pairs){
        if(pairs.length%2!=0)throw new IllegalArgumentException("Unpaired field");
        Map<String,Object> out=new LinkedHashMap<>();
        for(int n=0;n<pairs.length;n+=2)out.put((String)pairs[n],pairs[n+1]);
        return out;
    }
    private static byte[] readBounded(InputStream in,int maximum) throws IOException {
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] buffer=new byte[8192];int count;
        while((count=in.read(buffer,0,Math.min(buffer.length,maximum+1-out.size())))!=-1){
            out.write(buffer,0,count);
            if(out.size()>maximum)throw new IOException("UC-LOG-003");
        }
        return out.toByteArray();
    }
    private static boolean token(String v){
        return v!=null&&!v.isEmpty()&&v.length()<=120&&v.matches("[A-Za-z0-9_.$:+-]+");
    }
    public static String deviceToken(String v){
        if(v==null)return "unknown";
        String s=v.replaceAll("[^A-Za-z0-9_.$:+-]","_");
        return s.isEmpty()?"unknown":s.substring(0,Math.min(80,s.length()));
    }
    private static String quote(String value){
        StringBuilder b=new StringBuilder("\"");
        for(int n=0;n<value.length();n++){
            char c=value.charAt(n);
            if(c=='"'||c=='\\')b.append('\\').append(c);
            else if(c<32)b.append(String.format(Locale.ROOT,"\\u%04x",(int)c));
            else b.append(c);
        }
        return b.append('"').toString();
    }
    public synchronized boolean event(Event event,Map<String,?> detail){
        Objects.requireNonNull(event);
        // Validate before touching disk: callers must not pass paths, URIs, payloads or messages.
        StringJoiner data=new StringJoiner(",");
        for(var entry:detail.entrySet()){
            if(!KEYS.contains(entry.getKey()))throw new IllegalArgumentException("Diagnostic field is not allowlisted");
            Object value=entry.getValue();String encoded;
            if(value instanceof Boolean||value instanceof Integer||value instanceof Long)encoded=value.toString();
            else if(value instanceof String&&token((String)value))encoded=quote((String)value);
            else throw new IllegalArgumentException("Diagnostic value must be a bounded token, integer or boolean");
            data.add(quote(entry.getKey())+":"+encoded);
        }
        if(!healthy||closed)return false;
        String line="{\"schema\":1,\"seq\":"+(++sequence)+",\"utc_ms\":"+System.currentTimeMillis()+
            ",\"elapsed_ms\":"+((System.nanoTime()-started)/1000000)+",\"thread\":"+Thread.currentThread().getId()+
            ",\"code\":"+quote(event.code)+",\"level\":"+quote(event.level)+",\"reason\":"+quote(event.reason)+
            ",\"cause_known\":"+event.known+",\"detail\":{"+data+"}}\n";
        byte[] bytes=line.getBytes(StandardCharsets.UTF_8);
        if(bytes.length>limit)throw new IllegalArgumentException("Diagnostic record exceeds limit");
        try {
            if(size+bytes.length>limit)rotate();
            output.write(bytes);output.flush();size+=bytes.length;return true;
        }catch(IOException error){healthy=false;failureCode="UC-LOG-001";return false;}
    }
    private void rotate() throws IOException {
        output.close();Files.deleteIfExists(directory.resolve("launcher.3.jsonl"));
        for(int n=2;n>=0;n--){
            Path from=directory.resolve("launcher."+n+".jsonl");
            if(Files.isSymbolicLink(from))throw new IOException("UC-LOG-002");
            if(Files.exists(from))Files.move(from,directory.resolve("launcher."+(n+1)+".jsonl"));
        }
        output=Files.newOutputStream(directory.resolve("launcher.0.jsonl"),StandardOpenOption.CREATE_NEW);size=0;
    }
    public void javaFailure(Throwable error){
        event(Event.JAVA_FAILURE,detail("exception_class",deviceToken(error.getClass().getName())));
        StackTraceElement[] stack=error.getStackTrace();
        for(int n=0;n<Math.min(stack.length,16);n++){
            StackTraceElement frame=stack[n];
            event(Event.JAVA_FRAME,detail("class",deviceToken(frame.getClassName()),
                "method",deviceToken(frame.getMethodName()),"line",frame.getLineNumber()));
        }
    }
    public synchronized void installUncaughtHandler(){
        if(installed!=null)return;
        previous=Thread.getDefaultUncaughtExceptionHandler();
        installed=(thread,error)->{
            try{javaFailure(error);}catch(Throwable ignored){/* Never mask the original failure. */}
            finally{if(previous!=null)previous.uncaughtException(thread,error);}
        };
        Thread.setDefaultUncaughtExceptionHandler(installed);
    }
    private List<Path> sessions() throws IOException {
        List<Path> all=new ArrayList<>();
        try(DirectoryStream<Path> entries=Files.newDirectoryStream(root)){
            for(Path path:entries)if(path.getFileName().toString().matches("[a-fA-F0-9-]{8,64}")
                &&Files.isDirectory(path,LinkOption.NOFOLLOW_LINKS)&&!Files.isSymbolicLink(path))all.add(path);
        }
        all.sort(Comparator.comparingLong((Path p)->p.toFile().lastModified()).reversed().thenComparing(Path::toString));
        return all;
    }
    private static List<String> names(){
        List<String> result=new ArrayList<>(Arrays.asList("manifest.json"));
        for(int n=0;n<4;n++)result.add("launcher."+n+".jsonl");
        for(int n=0;n<4;n++)result.add("runtime."+n+".jsonl");
        return result;
    }
    private static void deleteOwned(Path path) throws IOException {
        if(Files.isSymbolicLink(path))return;
        // Unknown files are left untouched; never recursively delete game/saves or arbitrary directories.
        try(DirectoryStream<Path> entries=Files.newDirectoryStream(path)){
            for(Path entry:entries){
                String name=entry.getFileName().toString();
                if(name.equals("native")&&Files.isDirectory(entry,LinkOption.NOFOLLOW_LINKS))deleteOwned(entry);
                else if(names().contains(name)||name.equals("session.open"))Files.deleteIfExists(entry);
            }
        }
        try{Files.delete(path);}catch(DirectoryNotEmptyException ignored){}
    }
    private void retain() throws IOException {
        List<Path> old=sessions();old.remove(directory);
        for(int n=MAX_SESSIONS-1;n<old.size();n++)deleteOwned(old.get(n));
    }
    /** Closes destination. Only allowlisted metadata files are exported; raw SDK logs are excluded. */
    public synchronized void exportZip(OutputStream destination) throws IOException {
        Objects.requireNonNull(destination);
        event(Event.EXPORT,Collections.emptyMap());
        List<Path> selected=new ArrayList<>(Arrays.asList(directory));
        for(Path path:sessions())if(!path.equals(directory)&&selected.size()<MAX_SESSIONS)selected.add(path);
        try(ZipOutputStream zip=new ZipOutputStream(destination,StandardCharsets.UTF_8)){
            writeEntry(zip,"REPORT.txt",("Urban Chaos diagnostic report, schema 1\n"+
                "Only metadata is included. session.open means no clean end, not proof of a crash.\n"+
                "Native files are captured independently; a live snapshot is not globally atomic.\n"+
                "Per-session maximum: 4 launcher + 4 runtime records files (1 MiB per file maximum).\n"+
                "No ISO, XBE, saves, raw SDK logs, exception messages or memory dumps.\n").getBytes(StandardCharsets.UTF_8));
            for(Path session:selected){
                String prefix="sessions/"+session.getFileName()+"/";
                writeEntry(zip,prefix+"state.json",("{\"launcher_unclean\":"+
                    Files.exists(session.resolve("session.open"),LinkOption.NOFOLLOW_LINKS)+
                    ",\"native_unclean\":"+Files.exists(session.resolve("native/session.open"),LinkOption.NOFOLLOW_LINKS)+"}\n")
                    .getBytes(StandardCharsets.UTF_8));
                copyAllowed(zip,session,prefix);
                Path nativePath=session.resolve("native");
                if(Files.isSymbolicLink(nativePath))throw new IOException("UC-LOG-002");
                if(Files.isDirectory(nativePath,LinkOption.NOFOLLOW_LINKS))copyAllowed(zip,nativePath,prefix+"native/");
            }
        } catch(IOException error){
            String reason="zip_io_unclassified";
            if("UC-LOG-002".equals(error.getMessage()))reason="UC-LOG-002";
            if("UC-LOG-003".equals(error.getMessage()))reason="UC-LOG-003";
            event(Event.EXPORT_FAILURE,detail("reason",reason));
            throw error;
        }
        event(Event.EXPORT_PACKED,Collections.emptyMap());
    }
    private static void copyAllowed(ZipOutputStream zip,Path source,String prefix) throws IOException {
        for(String name:names()){
            Path path=source.resolve(name);
            if(Files.isSymbolicLink(path))throw new IOException("UC-LOG-002");
            if(!Files.isRegularFile(path,LinkOption.NOFOLLOW_LINKS))continue;
            int maximum=name.equals("manifest.json")?65536:MAX_FILE;
            try(InputStream input=Files.newInputStream(path,LinkOption.NOFOLLOW_LINKS)){
                byte[] bytes=readBounded(input,maximum);
                if(bytes.length>maximum)throw new IOException("UC-LOG-003");
                writeEntry(zip,prefix+name,bytes);
            }
        }
    }
    private static void writeEntry(ZipOutputStream zip,String name,byte[] data) throws IOException {
        ZipEntry entry=new ZipEntry(name);entry.setTime(0);zip.putNextEntry(entry);zip.write(data);zip.closeEntry();
    }
    @Override public synchronized void close() throws IOException {
        if(closed)return;
        boolean ended=event(Event.END,Collections.emptyMap());closed=true;
        try {
            if(output!=null)output.close();
            if(ended)Files.deleteIfExists(directory.resolve("session.open"));
        } finally {
            if(installed!=null&&Thread.getDefaultUncaughtExceptionHandler()==installed)
                Thread.setDefaultUncaughtExceptionHandler(previous);
        }
    }
}
