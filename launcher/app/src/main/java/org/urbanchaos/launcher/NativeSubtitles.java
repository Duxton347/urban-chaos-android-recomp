package org.urbanchaos.launcher;
import android.content.Context;
import android.util.AtomicFile;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.*;
import org.urbanchaos.launcher.localization.SubtitleArchive;
/** Optional private dictionaries modify native caption banks, preserving their original timing data. */
final class NativeSubtitles {
 static final String ASSET="localization/hope-street-ptbr.bin";
 static boolean available(Context c){try(InputStream in=c.getAssets().open(ASSET)){return true;}catch(IOException missing){return false;}}
 private static void check(boolean ok)throws IOException{if(!ok)throw new IOException("UC-LOCALE-001: pacote de legendas incompatível");}
 private static int number(DataInputStream in,int max)throws IOException{int n=in.readInt();check(n>=0&&n<=max);return n;}
 private static String text(DataInputStream in)throws IOException{int n=number(in,65536);byte[] bytes=new byte[n];in.readFully(bytes);return new String(bytes,StandardCharsets.UTF_8);}
 private static void write(File file,byte[] bytes)throws IOException{
  AtomicFile atomic=new AtomicFile(file);FileOutputStream out=null;
  try{out=atomic.startWrite();out.write(bytes);atomic.finishWrite(out);}catch(IOException error){if(out!=null)atomic.failWrite(out);throw error;}
 }
 private static byte[] withProfile(byte[] pack,boolean translate,boolean stored){byte[] profiled=Arrays.copyOf(pack,pack.length+2);profiled[pack.length]=(byte)(translate?1:0);profiled[pack.length+1]=(byte)(stored?1:0);return profiled;}
 static String prepare(Context c,String iso,boolean translate,boolean stored)throws IOException{
  check(translate||stored);
  byte[] pack;
  try(InputStream in=c.getAssets().open(ASSET);ByteArrayOutputStream out=new ByteArrayOutputStream()){
   byte[] buf=new byte[8192];int n;while((n=in.read(buf))!=-1){check(out.size()+n<=2*1024*1024);out.write(buf,0,n);}pack=out.toByteArray();
  }
  File directory=new File(c.getFilesDir(),"config/localization/"+SubtitleArchive.sha(withProfile(pack,translate,stored)));check(directory.isDirectory()||directory.mkdirs());
  StringBuilder manifest=new StringBuilder();Set<String> seen=new HashSet<>();
  try(DataInputStream in=new DataInputStream(new ByteArrayInputStream(pack))){
   byte[] magic=new byte[8];in.readFully(magic);check(Arrays.equals(magic,"UCPTBR01".getBytes(StandardCharsets.US_ASCII)));int archives=number(in,3);check(archives>0);
   for(int a=0;a<archives;a++){
    String name=text(in),entry=text(in),archiveSha=text(in),bankSha=text(in);
    check(Arrays.asList("globallanguage.lst","99-01.ent","12-01.ent").contains(name)&&seen.add(name)&&archiveSha.matches("[0-9a-f]{64}")&&bankSha.matches("[0-9a-f]{64}"));
    int count=number(in,10000);Map<String,String> translations=new LinkedHashMap<>();
    for(int n=0;n<count;n++){String key=text(in);int variant=number(in,255);check(translations.put(SubtitleArchive.identity(key,variant),text(in))==null);}
    if(!translate&&(!stored||!name.equals("12-01.ent")))continue;
    // Read from the user's mounted ISO; no original archive is part of this APK.
    byte[] source=NativeGame.readTextArchive(iso,name);check(source!=null&&SubtitleArchive.sha(source).equals(archiveSha));
    byte[] patched=translate?SubtitleArchive.patch(source,entry,bankSha,translations):source;
    if(stored&&name.equals("12-01.ent"))patched=SubtitleArchive.storedResources(patched);String sha=SubtitleArchive.sha(patched);File cached=new File(directory,name);
    if(!cached.isFile()||cached.length()!=patched.length||!SubtitleArchive.sha(Files.readAllBytes(cached.toPath())).equals(sha))write(cached,patched);
    manifest.append(name).append(' ').append(sha).append('\n');
   }
   check(in.read()==-1);
  }
  write(new File(directory,"verified.txt"),manifest.toString().getBytes(StandardCharsets.US_ASCII));return directory.getAbsolutePath();
 }
}
