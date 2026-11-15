package org.urbanchaos.diagnostic;
import java.io.*;import java.nio.file.*;
public class XisoTest {
 static int passed=0;
 static void rejects(File src,File dst,String expected)throws Exception {try{new Xiso(src,dst,()->{}).extract();throw new AssertionError("Expected rejection");}catch(IOException e){if(!e.getMessage().contains(expected))throw e;passed++;}}
 public static void main(String[] args)throws Exception {
  File root=new File(args[0]);File output=new File(root,"extracted");output.mkdirs();new Xiso(new File(root,"good.iso"),output,()->{}).extract();if(!new File(output,"Default.xbe").isFile())throw new AssertionError();passed++;
  rejects(new File(root,"ps2.iso"),output,"PS2 detectada");rejects(new File(root,"escape.iso"),output,"Caminho inseguro");rejects(new File(root,"truncated.iso"),output,"truncada");
  try{new Xiso(new File(root,"good.iso"),output,()->{throw new IOException("cancelled");}).extract();throw new AssertionError();}catch(IOException e){if(!e.getMessage().equals("cancelled"))throw e;passed++;}
  for(String name:new String[]{"../escape","..","","C:game","a\\b","a\nb"}){try{StoragePaths.name(name);throw new AssertionError();}catch(IOException e){passed++;}}
  System.out.println("Java XDVDFS/path/cancellation: "+passed+" checks passed");
 }
}
