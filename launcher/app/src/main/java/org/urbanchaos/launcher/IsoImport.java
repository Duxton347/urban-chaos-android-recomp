package org.urbanchaos.launcher;
import android.content.Context;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import org.json.JSONObject;
import org.urbanchaos.launcher.importer.ResumableCopy;
import org.urbanchaos.launcher.diagnostics.ReportSession;
import static org.urbanchaos.launcher.diagnostics.ReportSession.detail;
import java.io.*;
import java.nio.channels.FileLock;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
final class IsoImport {
 interface Status {void update(String message);}
 ReportSession reports;
 void event(ReportSession.Event kind,String code){if(reports!=null)reports.event(kind,detail("reason",code));}
 final Context context;final File folder,journal,partial,active;volatile boolean cancelled;
 IsoImport(Context context){this.context=context.getApplicationContext();folder=new File(context.getFilesDir(),"game");folder.mkdirs();journal=new File(folder,"import.json");partial=new File(folder,"import.iso.part");active=new File(folder,"active.json");}
 static void syncDirectory(File folder)throws IOException {try{java.io.FileDescriptor fd=android.system.Os.open(folder.getAbsolutePath(),android.system.OsConstants.O_RDONLY,0);try{android.system.Os.fsync(fd);}finally{android.system.Os.close(fd);}}catch(android.system.ErrnoException error){throw new IOException("UC-IMPORT-SYNC");}}
 static void atomic(File file,String text)throws IOException {File tmp=new File(file+".tmp");try(FileOutputStream out=new FileOutputStream(tmp)){out.write(text.getBytes(StandardCharsets.UTF_8));out.getFD().sync();}Files.move(tmp.toPath(),file.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);syncDirectory(file.getParentFile());}
 static JSONObject read(File file)throws Exception {if(file.length()>16384||Files.isSymbolicLink(file.toPath()))throw new IOException("UC-IMPORT-STATE");return new JSONObject(new String(Files.readAllBytes(file.toPath()),StandardCharsets.UTF_8));}
 File target(String name)throws IOException {if(!name.matches("installation-[a-f0-9-]+\\.iso"))throw new IOException("UC-IMPORT-PATH");File file=new File(folder,name);if(Files.isSymbolicLink(file.toPath())||!file.getCanonicalFile().getParentFile().equals(folder.getCanonicalFile()))throw new IOException("UC-IMPORT-PATH");return file;}
 File selected(){try{File file=target(read(active).getString("file"));return file.isFile()?file:null;}catch(Exception error){return null;}}
 void run(Uri selected,Status status){
  event(ReportSession.Event.IMPORT_START,selected==null?"resume":"selected");
  try(RandomAccessFile lockFile=new RandomAccessFile(new File(folder,"import.lock"),"rw");FileLock lock=lockFile.getChannel().tryLock()){
   if(lock==null)throw new IOException("UC-IMPORT-BUSY");
   JSONObject state;
   if(journal.exists()){state=read(journal);if(selected!=null){state.put("uri",selected.toString());atomic(journal,state.toString());}}
   else {if(selected==null)throw new IOException("UC-IMPORT-NO-PENDING");if(partial.exists()&&!partial.delete())throw new IOException("UC-IMPORT-STAGING");state=new JSONObject().put("uri",selected.toString());atomic(journal,state.toString());}
   if(!state.has("target")){
    Uri uri=Uri.parse(state.getString("uri"));
    try(ParcelFileDescriptor descriptor=context.getContentResolver().openFileDescriptor(uri,"r")){
     if(descriptor==null)throw new IOException("UC-IMPORT-OPEN");long size=descriptor.getStatSize();
     if(state.has("size")&&state.getLong("size")!=size)throw new IOException("UC-IMPORT-SOURCE-CHANGED");
     state.put("size",size);atomic(journal,state.toString());
     try(InputStream input=new ParcelFileDescriptor.AutoCloseInputStream(descriptor)){
      ResumableCopy.copy(input,partial,size,folder.getUsableSpace(),new ResumableCopy.State(){long previous;
       public boolean cancelled(){return cancelled;}
       public void progress(long done,long total,boolean verifying){long now=System.nanoTime();if(now-previous>250000000||done==total){previous=now;if(reports!=null)reports.event(ReportSession.Event.IMPORT_PROGRESS,detail("bytes",done,"total",total,"mode",verifying?"verify":"copy"));status.update((verifying?"Verificando retomada: ":"Copiando ISO: ")+done/1048576+" / "+total/1048576+" MiB");}}
      });
     }
    }
    if(cancelled)throw new ResumableCopy.Paused();status.update("Validando a imagem Xbox…");NativeGame.load();
    String code=NativeGame.validate(partial.getAbsolutePath());if(!"UC-IMPORT-000".equals(code))throw new IOException(code);
    state.put("target","installation-"+UUID.randomUUID()+".iso");atomic(journal,state.toString());
   }
   File target=target(state.getString("target"));
   if(cancelled)throw new ResumableCopy.Paused();
   if(!target.exists()){Files.move(partial.toPath(),target.toPath(),StandardCopyOption.ATOMIC_MOVE);syncDirectory(folder);}
   NativeGame.load();String code=NativeGame.validate(target.getAbsolutePath());if(!"UC-IMPORT-000".equals(code))throw new IOException(code);
   atomic(active,new JSONObject().put("file",target.getName()).toString());journal.delete();event(ReportSession.Event.IMPORT_READY,"validated");status.update("ISO validada. Pronto para iniciar o teste experimental.");
  }catch(Exception|LinkageError error){String code=error instanceof ResumableCopy.Paused?"UC-IMPORT-PAUSED":error.getMessage();
   if(code==null||!code.matches("[A-Za-z0-9_-]{1,80}"))code="UC-IMPORT-FAILED";
   event(error instanceof ResumableCopy.Paused?ReportSession.Event.IMPORT_CANCELLED:ReportSession.Event.IMPORT_FAILED,code);status.update(message(code));
  }
 }
 void discard(Status status){try(RandomAccessFile lockFile=new RandomAccessFile(new File(folder,"import.lock"),"rw");FileLock lock=lockFile.getChannel().tryLock()){if(lock==null)throw new IOException("UC-IMPORT-BUSY");Files.deleteIfExists(partial.toPath());Files.deleteIfExists(journal.toPath());status.update("Importação pendente descartada. Instalação ativa e saves preservados.");}catch(Exception error){status.update("Não foi possível descartar agora (UC-IMPORT-BUSY).");}}
 static String message(String code){
  if(code.equals("UC-IMPORT-RESUME-MISMATCH"))return "A ISO não coincide com a cópia pendente. Selecione a mesma ISO ou descarte a importação pendente.";
  if(code.equals("UC-IMPORT-SPACE"))return "Espaço livre insuficiente para copiar a ISO com margem de segurança.";
  if(code.equals("UC-IMPORT-SIZE"))return "Tamanho da ISO indisponível ou inválido. Selecione um arquivo ISO salvo no tablet.";
  if(code.equals("UC-IMPORT-PAUSED"))return "Importação pausada. Use Retomar para continuar.";
  if(code.equals("UC-IMPORT-REVISION"))return "Revisão Xbox diferente da compilada. Importação não ativada; a instalação anterior foi preservada.";
  if(code.equals("compressed_image"))return "Selecione a ISO descompactada, não ZIP/RAR/7z.";
  if(code.equals("ps2_image"))return "Esta imagem é de PS2. Selecione a edição Xbox.";
  return "Importação não concluída ("+code+"). A instalação anterior foi preservada.";
 }
}
