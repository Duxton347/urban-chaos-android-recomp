import org.urbanchaos.launcher.diagnostics.ReportSession;
import static org.urbanchaos.launcher.diagnostics.ReportSession.detail;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.zip.*;

public class DiagnosticsTest {
 static int checks;
 static String hash=String.join("",Collections.nCopies(64,"a"));
 static Path root,nativeRoot;
 interface Task {void run() throws Exception;}
 static void check(boolean condition){checks++;if(!condition)throw new AssertionError("diagnostic check "+checks);}
 static void rejects(Class<? extends Throwable> type,Task action) throws Exception {
  try{action.run();throw new AssertionError("Expected "+type.getName());}
  catch(Throwable error){if(!type.isInstance(error))throw error;}checks++;
 }
 static Map<String,byte[]> unzip(byte[] data) throws IOException {
  Map<String,byte[]> out=new LinkedHashMap<>();
  try(ZipInputStream zip=new ZipInputStream(new ByteArrayInputStream(data))){
   ZipEntry entry;byte[] buffer=new byte[4096];
   while((entry=zip.getNextEntry())!=null){
    ByteArrayOutputStream bytes=new ByteArrayOutputStream();int n;
    while((n=zip.read(buffer))!=-1)bytes.write(buffer,0,n);
    out.put(entry.getName(),bytes.toByteArray());
   }
  }
  return out;
 }
 static String text(byte[] b){return new String(b,StandardCharsets.UTF_8);}
 static ReportSession session(String name,int limit) throws IOException {
  return new ReportSession(root.resolve(name).toFile(),"fixture-build",hash,limit);
 }
 public static void main(String[] args) throws Exception {
  root=Paths.get(args[0]);nativeRoot=Paths.get(args[1]);Files.createDirectories(root);
  try(ReportSession s=session("basic",4096)){
   check(s.event(ReportSession.Event.IMPORT_PROGRESS,detail("bytes",12L,"total",100L)));
   String log=text(Files.readAllBytes(s.directory().toPath().resolve("launcher.0.jsonl")));
   check(log.contains("UC-IMPORT-002")&&log.contains("\"bytes\":12"));
   rejects(IllegalArgumentException.class,()->s.event(ReportSession.Event.IMPORT_FAILED,detail("content","SENTINEL_GAME_BYTES")));
   rejects(IllegalArgumentException.class,()->s.event(ReportSession.Event.IMPORT_FAILED,detail("state","/private/user/file.iso")));
   s.javaFailure(new RuntimeException("SENTINEL_EXCEPTION_MESSAGE"));
   check(!text(Files.readAllBytes(s.directory().toPath().resolve("launcher.0.jsonl"))).contains("SENTINEL_EXCEPTION_MESSAGE"));
   check(s.healthy());
  }
  Path clean;
  try(ReportSession s=session("clean",4096)){clean=s.directory().toPath();}
  check(!Files.exists(clean.resolve("session.open")));
  try(ReportSession s=session("rotation",4096)){
   for(int n=0;n<300;n++)check(s.event(ReportSession.Event.IMPORT_PROGRESS,detail("counter",n)));
   int files=0;
   for(int n=0;n<4;n++){Path f=s.directory().toPath().resolve("launcher."+n+".jsonl");if(Files.exists(f)){files++;check(Files.size(f)<=4096);}}
   check(files==4);
  }
  ReportSession old=session("unclean",4096);
  try(ReportSession next=session("unclean",4096)){
   String log=text(Files.readAllBytes(next.directory().toPath().resolve("launcher.0.jsonl")));
   check(log.contains("UC-APP-003")&&log.contains("\"cause_known\":false"));
  }finally{old.close();}
  for(int n=0;n<5;n++)try(ReportSession s=session("retention",4096)){}
  try(var entries=Files.list(root.resolve("retention"))){check(entries.filter(Files::isDirectory).count()==3);}
  Thread.UncaughtExceptionHandler previous=Thread.getDefaultUncaughtExceptionHandler();
  AtomicInteger delegated=new AtomicInteger();
  Thread.UncaughtExceptionHandler sentinel=(thread,error)->delegated.incrementAndGet();
  Thread.setDefaultUncaughtExceptionHandler(sentinel);
  try(ReportSession s=session("handler",4096)){
   s.installUncaughtHandler();
   Thread.getDefaultUncaughtExceptionHandler().uncaughtException(Thread.currentThread(),new RuntimeException("SENTINEL_EXCEPTION_MESSAGE"));
   check(delegated.get()==1);
  }finally{check(Thread.getDefaultUncaughtExceptionHandler()==sentinel);Thread.setDefaultUncaughtExceptionHandler(previous);}
  try(ReportSession s=session("export",4096)){
   Files.writeString(s.directory().toPath().resolve("game.iso"),"SENTINEL_GAME_BYTES");
   Files.writeString(s.directory().toPath().resolve("raw.log"),"SENTINEL_PRIVATE_FOLDER");
   Path nativePath=s.nativeDirectory().toPath();Files.createDirectories(nativePath);
   for(String name:Arrays.asList("manifest.json","runtime.0.jsonl"))
    Files.copy(nativeRoot.resolve("standard").resolve(name),nativePath.resolve(name));
   Files.createFile(nativePath.resolve("session.open"));
   ByteArrayOutputStream archive=new ByteArrayOutputStream();s.exportZip(archive);
   Map<String,byte[]> entries=unzip(archive.toByteArray());
   check(entries.keySet().stream().noneMatch(p->p.contains("game.iso")||p.contains("raw.log")));
   check(entries.keySet().stream().anyMatch(p->p.endsWith("native/runtime.0.jsonl")));
   String all=entries.values().stream().map(DiagnosticsTest::text).reduce("",String::concat);
   check(!all.contains("SENTINEL_"));
   check(all.contains("\"native_unclean\":true"));
   Files.write(root.resolve("report-fixture.zip"),archive.toByteArray());
   rejects(IOException.class,()->s.exportZip(new OutputStream(){public void write(int b) throws IOException{throw new IOException("destination full");}}));
   String failed=text(Files.readAllBytes(s.directory().toPath().resolve("launcher.0.jsonl")));
   check(failed.contains("UC-EXPORT-002"));check(failed.contains("zip_io_unclassified"));
  }
  try(ReportSession s=session("symlink",4096)){
   Path outside=root.resolve("outside");Files.writeString(outside,"SENTINEL_GAME_BYTES");
   Files.createSymbolicLink(s.directory().toPath().resolve("runtime.0.jsonl"),outside.toAbsolutePath());
   rejects(IOException.class,()->s.exportZip(new ByteArrayOutputStream()));
  }
  try(ReportSession s=session("oversized",4096)){
   try(RandomAccessFile file=new RandomAccessFile(new File(s.directory(),"runtime.0.jsonl"),"rw")){file.setLength(1024*1024+1);}
   rejects(IOException.class,()->s.exportZip(new ByteArrayOutputStream()));
  }
  try(ReportSession s=session("parallel",ReportSession.DEFAULT_LIMIT)){
   List<Thread> workers=new ArrayList<>();
   for(int n=0;n<4;n++){Thread thread=new Thread(()->{for(int m=0;m<25;m++)s.event(ReportSession.Event.IMPORT_PROGRESS,detail("counter",m));});workers.add(thread);thread.start();}
   for(Thread worker:workers)worker.join();
   List<String> lines=Files.readAllLines(s.directory().toPath().resolve("launcher.0.jsonl"));check(lines.size()==101);
  }
  System.out.println("Java diagnostic checks passed: "+checks+"; authored fixtures, no game execution");
 }
}
