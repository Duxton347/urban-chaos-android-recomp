package org.urbanchaos.diagnostic;
import android.app.*;
import android.os.*;
import android.content.*;
import android.net.Uri;
import android.widget.*;
import java.io.*;
import java.util.concurrent.*;
public class MainActivity extends Activity {
 TextView status; Importer importer; static final ExecutorService worker=Executors.newSingleThreadExecutor();
 void button(LinearLayout l,String text,Runnable action){Button b=new Button(this);b.setText(text);b.setOnClickListener(v->action.run());l.addView(b);}
 @Override public void onCreate(Bundle b){super.onCreate(b);LinearLayout l=new LinearLayout(this);l.setOrientation(LinearLayout.VERTICAL);l.setPadding(24,24,24,24);ScrollView scroll=new ScrollView(this);scroll.addView(l);setContentView(scroll);
 TextView title=new TextView(this);title.setText("URBAN CHAOS — DIAGNÓSTICO\nNão executa o jogo. Frames de teste Vulkan e importação local.");title.setTextSize(22);l.addView(title);
 status=new TextView(this);l.addView(status);
 for(String d:new String[]{"game","saves","config","cache/shaders","logs"})new File(getFilesDir(),d).mkdirs();
 importer=new Importer(this,s->runOnUiThread(()->status.setText(s)));
 button(l,"Testar Vulkan / SDL3",()->startActivity(new Intent(this,DiagnosticActivity.class)));
 button(l,"Importar arquivo Xbox (XBE ou XISO)",()->{Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT);i.setType("*/*");i.addCategory(Intent.CATEGORY_OPENABLE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);startActivityForResult(i,1);});
 button(l,"Importar pasta Xbox",()->{Intent i=new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);startActivityForResult(i,2);});
 button(l,"Pausar importação",()->importer.cancel());button(l,"Retomar importação",()->worker.submit(()->importer.resume()));button(l,"Descartar importação incompleta",()->{importer.cancel();worker.submit(()->importer.discard());});
 button(l,"Editor de HUD de diagnóstico",()->startActivity(new Intent(this,HudActivity.class)));
 button(l,"Exportar relatório do dispositivo",()->{Intent i=new Intent(Intent.ACTION_CREATE_DOCUMENT);i.setType("text/plain");i.putExtra(Intent.EXTRA_TITLE,"urban-chaos-diagnostic.txt");startActivityForResult(i,3);});
 status.setText("Importações interrompidas podem ser retomadas. O jogo permanece bloqueado em G1.");}
 @Override protected void onActivityResult(int r,int c,Intent d){super.onActivityResult(r,c,d);if(c!=RESULT_OK||d==null)return;Uri u=d.getData();if(u==null)return;
 if(r==3){worker.submit(()->{try(OutputStream o=getContentResolver().openOutputStream(u)){String s="DIAGNOSTIC ONLY\nmodel="+Build.MANUFACTURER+" "+Build.MODEL+"\nAndroid="+Build.VERSION.RELEASE+" SDK="+Build.VERSION.SDK_INT+"\n";o.write(s.getBytes(java.nio.charset.StandardCharsets.UTF_8));File dir=new File(getFilesDir(),"logs");for(File f:dir.listFiles()==null?new File[0]:dir.listFiles())if(f.isFile()&&f.length()<1048576)try(InputStream in=new FileInputStream(f)){byte[] buf=new byte[8192];int n;while((n=in.read(buf))!=-1)o.write(buf,0,n);}File sdl=new File(getFilesDir(),"vulkan.log");if(sdl.exists())try(InputStream in=new FileInputStream(sdl)){byte[] buf=new byte[8192];int n;while((n=in.read(buf))!=-1)o.write(buf,0,n);}runOnUiThread(()->status.setText("Relatório exportado. Confirme os resultados manuais no documento."));}catch(Exception e){runOnUiThread(()->status.setText(e.toString()));}});return;}
 try{getContentResolver().takePersistableUriPermission(u,Intent.FLAG_GRANT_READ_URI_PERMISSION);worker.submit(()->importer.start(u,r==2));}catch(Exception e){status.setText("Permissão SAF persistente necessária: "+e.getMessage());}}
 @Override protected void onDestroy(){if(importer!=null)importer.cancel();super.onDestroy();}
}
