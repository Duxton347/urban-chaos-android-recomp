import org.urbanchaos.launcher.localization.SubtitleArchive;
import java.nio.file.*;
import java.io.*;
import java.util.*;
public final class SubtitleArchiveTest {
 static int checks;
 interface Work {void run()throws Exception;}
 static void rejects(Work work)throws Exception{checks++;try{work.run();throw new AssertionError("Accepted invalid archive");}catch(IOException expected){}}
 public static void main(String[] args)throws Exception{
  byte[] input=Files.readAllBytes(Path.of(args[0]));String sha=args[1];Map<String,String> text=new LinkedHashMap<>();text.put(SubtitleArchive.identity("dialogue",0),"Olá, mundo");text.put(SubtitleArchive.identity("dialogue",1),"Aperte \u0002");
  byte[] result=SubtitleArchive.patch(input,"authored.exttxt",sha,text);Files.write(Path.of(args[2]),result);checks++;
  rejects(()->SubtitleArchive.patch(input,"absent",sha,text));
  rejects(()->SubtitleArchive.patch(input,"authored.exttxt","0".repeat(64),text));
  Map<String,String> invalid=new HashMap<>(text);invalid.put(SubtitleArchive.identity("dialogue",1),"Aperte");rejects(()->SubtitleArchive.patch(input,"authored.exttxt",sha,invalid));
  invalid.put(SubtitleArchive.identity("dialogue",1),"🎮");rejects(()->SubtitleArchive.patch(input,"authored.exttxt",sha,invalid));
  byte[] corrupt=input.clone();corrupt[corrupt.length-1]^=1;rejects(()->SubtitleArchive.patch(corrupt,"authored.exttxt",sha,text));
  if(args.length>3){byte[] first=Files.readAllBytes(Path.of(args[3]));rejects(()->SubtitleArchive.patch(first,"authored.exttxt",sha,text));}
  System.out.println("Native caption archive Java checks passed: "+checks+"; authored data only");
 }
}
