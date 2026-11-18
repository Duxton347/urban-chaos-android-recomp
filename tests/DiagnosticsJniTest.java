import org.urbanchaos.launcher.diagnostics.ReportSession;
import org.urbanchaos.launcher.diagnostics.NativeReports;
import java.nio.file.*;
import java.util.*;
import java.io.*;
import java.util.zip.*;

public class DiagnosticsJniTest {
 private static native void emitFixture();
 static void require(boolean ok){if(!ok)throw new AssertionError("JNI diagnostic fixture");}
 public static void main(String[] args) throws Exception {
  System.load(Paths.get(args[0]).toAbsolutePath().toString());
  try(ReportSession s=new ReportSession(Paths.get(args[1]).toFile(),"jni-fixture",String.join("",Collections.nCopies(64,"a")))){
   require(NativeReports.attach(s,"jni-fixture",String.join("",Collections.nCopies(64,"a")),false));
   require(NativeReports.healthy());emitFixture();NativeReports.detach();
   require(!NativeReports.healthy());require(!new File(s.nativeDirectory(),"session.open").exists());
   String nativeLog=Files.readString(new File(s.nativeDirectory(),"runtime.0.jsonl").toPath());
   require(nativeLog.contains("UC-GPU-001")&&nativeLog.contains("\"ordinal\":233"));
   require(nativeLog.contains("UC-GPU-002")&&nativeLog.contains("\"frame\":1800")&&nativeLog.contains("\"samples\":64"));
   require(nativeLog.contains("UC-GPU-003")&&nativeLog.contains("UC-GPU-004")&&nativeLog.contains("\"frame\":2400")&&nativeLog.contains("\"headless\":0"));
   require(!nativeLog.contains("SENTINEL_"));
   try(OutputStream zip=Files.newOutputStream(Paths.get(args[2]))){s.exportZip(zip);}
  }
  System.out.println("JNI diagnostics passed: shared SDK observer -> native journal -> Java export");
 }
}
