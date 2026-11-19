import org.urbanchaos.launcher.importer.ResumableCopy;
import java.io.*;
import java.nio.file.*;
import java.util.*;
public final class ResumableCopyTest {
 static int checks;static void require(boolean condition){checks++;if(!condition)throw new AssertionError("ISO copy contract");}
 static final class State implements ResumableCopy.State {
  boolean pause;long stop=Long.MAX_VALUE;long verified;
  public boolean cancelled(){return pause;}
  public void progress(long bytes,long total,boolean verifying){if(verifying)verified=bytes;else if(bytes>=stop)pause=true;}
 }
 interface Call {void run()throws Exception;}
 static void fail(String code,Call call)throws Exception {try{call.run();throw new AssertionError("Expected "+code);}catch(IOException expected){require(expected.getMessage().equals(code));}}
 public static void main(String[] args)throws Exception {
  Path root=Files.createTempDirectory(Paths.get(args[0]),"copy-fixture-");
  byte[] bytes=new byte[3*1024*1024+13];new Random(8137).nextBytes(bytes);long free=bytes.length+64L*1024*1024;
  File partial=root.resolve("partial.iso").toFile();State stopped=new State();stopped.stop=1024*1024;
  fail("UC-IMPORT-PAUSED",()->ResumableCopy.copy(new ByteArrayInputStream(bytes),partial,bytes.length,free,stopped));
  require(partial.length()==1024*1024);
  byte[] changed=bytes.clone();changed[100]^=1;State wrong=new State();
  fail("UC-IMPORT-RESUME-MISMATCH",()->ResumableCopy.copy(new ByteArrayInputStream(changed),partial,changed.length,free,wrong));require(partial.length()==1024*1024);
  State resume=new State();ResumableCopy.copy(new ByteArrayInputStream(bytes),partial,bytes.length,free,resume);require(resume.verified==1024*1024);require(Arrays.equals(bytes,Files.readAllBytes(partial.toPath())));
  State done=new State();ResumableCopy.copy(new ByteArrayInputStream(bytes),partial,bytes.length,64L*1024*1024,done);require(done.verified==bytes.length);
  File empty=root.resolve("space.iso").toFile();fail("UC-IMPORT-SPACE",()->ResumableCopy.copy(new ByteArrayInputStream(bytes),empty,bytes.length,free-1,new State()));require(!empty.exists());
  fail("UC-IMPORT-SIZE",()->ResumableCopy.copy(new ByteArrayInputStream(bytes),empty,-1,free,new State()));
  File truncated=root.resolve("truncated.iso").toFile();fail("UC-IMPORT-TRUNCATED",()->ResumableCopy.copy(new ByteArrayInputStream(new byte[17]),truncated,bytes.length,free,new State()));require(truncated.length()==0);
  File extra=root.resolve("extra.iso").toFile();fail("UC-IMPORT-SOURCE-CHANGED",()->ResumableCopy.copy(new ByteArrayInputStream(bytes),extra,bytes.length-1,free,new State()));
  require(extra.length()==bytes.length-1);
  try(java.util.stream.Stream<Path> files=Files.walk(root)){for(Path path:(Iterable<Path>)files.sorted(Comparator.reverseOrder())::iterator)Files.delete(path);}
  System.out.println("ISO resumable copy checks passed: "+checks+"; authored bytes only");
 }
}
