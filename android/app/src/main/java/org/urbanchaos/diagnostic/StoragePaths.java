package org.urbanchaos.diagnostic;
import java.io.IOException;
final class StoragePaths {
 static String name(String s)throws IOException {
  if(s==null||s.isEmpty()||s.equals(".")||s.equals(".."))throw new IOException("Caminho inseguro");
  for(int i=0;i<s.length();i++){char c=s.charAt(i);if(c<32||c=='/'||c=='\\'||c==':'||c==127)throw new IOException("Caminho inseguro");}
  return s;
 }
}
