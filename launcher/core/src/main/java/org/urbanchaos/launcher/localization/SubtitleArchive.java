package org.urbanchaos.launcher.localization;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import java.util.regex.Pattern;
import java.util.zip.*;

/** Applies translated strings to native caption banks. No audio/video clocks or original assets included. */
public final class SubtitleArchive {
 public static final int LIMIT=64*1024*1024;
 private static final Pattern FORMAT=Pattern.compile("%(?:\\d+\\$)?[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?(?:hh|h|ll|l|L|z|t|j)?[diuoxXfFeEgGaAcspn%]");
 private SubtitleArchive(){}
 private static void check(boolean ok,String reason)throws IOException{if(!ok)throw new IOException(reason);}
 private static void span(int at,int n,int end)throws IOException{check(at>=0&&n>=0&&at<=end-n,"extent");}
 private static int u16(byte[] b,int at)throws IOException{span(at,2,b.length);return (b[at]&255)|((b[at+1]&255)<<8);}
 private static int u32(byte[] b,int at)throws IOException{span(at,4,b.length);return (b[at]&255)|((b[at+1]&255)<<8)|((b[at+2]&255)<<16)|(b[at+3]<<24);}
 private static void put(byte[] b,int at,int n){for(int i=0;i<4;i++)b[at+i]=(byte)(n>>>(i*8));}
 public static byte[] digest(String algorithm,byte[] data,int count)throws IOException{
  try{MessageDigest hash=MessageDigest.getInstance(algorithm);hash.update(data,0,count);return hash.digest();}catch(java.security.GeneralSecurityException error){throw new IOException("digest",error);}
 }
 public static String sha(byte[] data)throws IOException{StringBuilder hex=new StringBuilder();for(byte b:digest("SHA-256",data,data.length))hex.append(String.format(java.util.Locale.ROOT,"%02x",b&255));return hex.toString();}
 public static String identity(String key,int variant){return key+"\n"+variant;}
 private static String zstring(byte[] b,int at,int end)throws IOException{span(at,1,end);int stop=at;while(stop<end&&b[stop]!=0)stop++;check(stop<end,"terminator");return new String(b,at,stop-at,StandardCharsets.ISO_8859_1);}
 private static String tokens(String text){StringBuilder result=new StringBuilder();for(char c:text.toCharArray())if(c<32||(c>=127&&c<160))result.append(c);return result.toString();}
 private static List<String> formats(String text){List<String> result=new ArrayList<>();java.util.regex.Matcher matcher=FORMAT.matcher(text);while(matcher.find())result.add(matcher.group());return result;}
 private static void replacement(String old,String text)throws IOException{
  check(!old.isEmpty()&&!text.isEmpty()&&text.length()<=4096,"caption_length");for(char c:text.toCharArray())check(c>0&&c<=255,"caption_character");
  check(tokens(old).equals(tokens(text))&&formats(old).equals(formats(text)),"caption_tokens");
 }
 private static byte[] inflate(byte[] input,int offset,int packed,int expected)throws IOException{
  span(offset,packed,input.length);check(expected>=0&&expected<=LIMIT,"inflate_limit");Inflater inflater=new Inflater();
  try{inflater.setInput(input,offset,packed);byte[] result=new byte[expected];int n=0;
   while(n<result.length){int got=inflater.inflate(result,n,result.length-n);if(got==0)break;n+=got;}
   byte[] extra=new byte[1];int overflow=inflater.inflate(extra);
   check(n==expected&&overflow==0&&inflater.finished()&&inflater.getRemaining()==0,"inflate_shape");return result;
  }catch(DataFormatException error){throw new IOException("inflate",error);}finally{inflater.end();}
 }
 private static byte[] deflate(byte[] raw)throws IOException{return deflate(raw,9);}
 private static byte[] deflate(byte[] raw,int level)throws IOException{
  Deflater encoder=new Deflater(level);try{encoder.setInput(raw);encoder.finish();ByteArrayOutputStream out=new ByteArrayOutputStream();byte[] chunk=new byte[8192];
   while(!encoder.finished()){int n=encoder.deflate(chunk);check(n>0&&out.size()<=LIMIT-n,"deflate_limit");out.write(chunk,0,n);}return out.toByteArray();
  }finally{encoder.end();}
 }
 public static byte[] translateBank(byte[] bank,Map<String,String> translations)throws IOException{
  check(bank.length>=24&&bank.length<=LIMIT,"bank_length");int languages=u16(bank,0),keys=u32(bank,4),meta=u32(bank,8),textSize=u32(bank,12),aux=u32(bank,16),count=u32(bank,20);
  long strideLong=(long)keys+meta+textSize+aux;check(languages>=1&&languages<=16&&count>0&&count<=65535&&keys==count*56&&meta>0&&meta%12==0&&textSize>0&&aux>=0&&strideLong>0&&strideLong<=LIMIT&&24+languages*strideLong==bank.length,"bank_shape");int stride=(int)strideLong;
  byte[] descriptors=Arrays.copyOfRange(bank,24+keys,24+keys+meta);ByteArrayOutputStream pool=new ByteArrayOutputStream();Map<String,Integer> offsets=new HashMap<>();Set<String> matched=new HashSet<>();Set<Integer> records=new HashSet<>();Set<String> names=new HashSet<>();
  for(int index=0;index<count;index++){
   int at=24+index*56;String name=zstring(bank,at,at+48);check(!name.isEmpty()&&names.add(name),"bank_key");int variants=bank[at+49]&255,first=u32(bank,at+52);
   check(variants>0&&first%12==0,"bank_variants");span(first,variants*12,meta);
   for(int variant=0;variant<variants;variant++){
    int record=first+variant*12;check(records.add(record),"bank_descriptor");int start=u32(bank,24+keys+record);span(start,1,textSize);
    int auxStart=u32(bank,24+keys+record+4),auxLength=u32(bank,24+keys+record+8);if(auxStart!=-1)span(auxStart,auxLength,aux);
    String source=zstring(bank,24+keys+meta+start,24+keys+meta+textSize),id=identity(name,variant);String translated=translations.get(id);
    if(translated!=null){replacement(source,translated);matched.add(id);}else translated=source;
    Integer offset=offsets.get(translated);if(offset==null){offset=pool.size();byte[] bytes=translated.getBytes(StandardCharsets.ISO_8859_1);check(pool.size()<=LIMIT-bytes.length-1,"pool_limit");pool.write(bytes);pool.write(0);offsets.put(translated,offset);}put(descriptors,record,offset);
   }
  }
  check(records.size()*12==meta&&matched.equals(translations.keySet()),"unknown_translation");int size=Math.max(textSize,pool.size());long length=24L+languages*(strideLong-textSize+size);check(length<=LIMIT,"translated_bank_limit");
  byte[] result=new byte[(int)length];System.arraycopy(bank,0,result,0,24);put(result,12,size);byte[] translated=pool.toByteArray();int dest=24;
  for(int lang=0;lang<languages;lang++){
   int from=24+lang*stride;System.arraycopy(bank,from,result,dest,keys);
   System.arraycopy(lang==0?descriptors:bank,lang==0?0:from+keys,result,dest+keys,meta);
   System.arraycopy(lang==0?translated:bank,lang==0?0:from+keys+meta,result,dest+keys+meta,lang==0?translated.length:textSize);
   System.arraycopy(bank,from+keys+meta+textSize,result,dest+keys+meta+size,aux);dest+=keys+meta+size+aux;
  }
  return result;
 }
 private static final class Entry {
  String name;int record,offset,raw,packed;
 }
 private static List<Entry> index(byte[] archive)throws IOException{
  check(archive.length>=208&&archive.length<=LIMIT&&archive.length%2048==16&&u32(archive,0)==28,"archive_header");
  check(MessageDigest.isEqual(digest("MD5",archive,archive.length-16),Arrays.copyOfRange(archive,archive.length-16,archive.length)),"archive_md5");
  for(int p=24;p<44;p+=4)check(u32(archive,p)==(archive.length-16)/2048,"archive_sectors");
  int table=u32(archive,192),count=u32(archive,196);check(table>=4&&table<=16*1024*1024&&count>0&&count<=100000,"archive_index");span(196,table,archive.length-16);
  int end=196+table,at=200;List<Entry> entries=new ArrayList<>();
  for(int i=0;i<count;i++){
   int length=u16(archive,at);at+=2;check(length>0&&length<=1024,"archive_name");span(at,length+16,end);Entry e=new Entry();e.name=zstring(archive,at,at+length);check(e.name.length()==length-1,"archive_name");at+=length;
   e.record=at;e.offset=u32(archive,at);e.raw=u32(archive,at+4);e.packed=u32(archive,at+8);span(e.offset,e.packed,archive.length-16);check(e.offset>=end&&e.raw>=0&&e.raw<=LIMIT,"archive_payload");at+=16;entries.add(e);
  }
  check(at==end,"archive_index_end");entries.sort(Comparator.comparingInt(e->e.offset));int previous=end;
  for(Entry e:entries){check(e.offset>=previous,"archive_overlap");previous=e.offset+e.packed;}
  for(int i=previous;i<archive.length-16;i++)check(archive[i]==0,"archive_trailer");return entries;
 }
 private static byte[] finish(byte[] prefix,byte[] compressed,int offset)throws IOException{
  long padded=((long)offset+compressed.length+2047)/2048*2048;check(padded+16<=LIMIT,"archive_output_limit");byte[] result=Arrays.copyOf(prefix,(int)padded+16);
  System.arraycopy(compressed,0,result,offset,compressed.length);for(int p=24;p<44;p+=4)put(result,p,(int)padded/2048);
  System.arraycopy(digest("MD5",result,(int)padded),0,result,(int)padded,16);return result;
 }
 public static byte[] patch(byte[] archive,String entry,String bankSha,Map<String,String> translations)throws IOException{
  List<Entry> entries=index(archive);Entry target=null;
  for(Entry e:entries)if(e.name.equals(entry)){check(target==null,"archive_ambiguous");target=e;}
  check(target!=null&&target==entries.get(entries.size()-1),"text_must_be_last");byte[] bank=inflate(archive,target.offset,target.packed,target.raw);check(sha(bank).equals(bankSha),"bank_revision");
  byte[] replaced=translateBank(bank,translations),compressed=deflate(replaced),prefix=Arrays.copyOf(archive,target.offset);
  put(prefix,target.record+4,replaced.length);put(prefix,target.record+8,compressed.length);return finish(prefix,compressed,target.offset);
 }
 /** Same raw resources and index identities, encoded as standard stored DEFLATE blocks for faster guest loading. */
 public static byte[] storedResources(byte[] archive)throws IOException{
  List<Entry> entries=index(archive);ByteArrayOutputStream out=new ByteArrayOutputStream();out.write(archive,0,entries.get(0).offset);
  int[] offsets=new int[entries.size()],sizes=new int[entries.size()];
  for(int i=0;i<entries.size();i++){
   Entry e=entries.get(i);byte[] raw=inflate(archive,e.offset,e.packed,e.raw),packed=deflate(raw,0);
   offsets[i]=out.size();sizes[i]=packed.length;check(out.size()<=LIMIT-packed.length,"archive_output_limit");out.write(packed);
   if(i+1<entries.size()){int gap=e.offset+e.packed,length=entries.get(i+1).offset-gap;check(out.size()<=LIMIT-length,"archive_output_limit");out.write(archive,gap,length);}
  }
  byte[] result=finish(out.toByteArray(),new byte[0],out.size());
  for(int i=0;i<entries.size();i++){put(result,entries.get(i).record,offsets[i]);put(result,entries.get(i).record+8,sizes[i]);}
  System.arraycopy(digest("MD5",result,result.length-16),0,result,result.length-16,16);return result;
 }
}
