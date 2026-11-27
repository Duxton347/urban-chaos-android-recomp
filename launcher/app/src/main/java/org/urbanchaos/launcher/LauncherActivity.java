package org.urbanchaos.launcher;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.*;
import org.urbanchaos.launcher.diagnostics.*;
import java.io.File;
import java.util.concurrent.*;
public final class LauncherActivity extends Activity {
 private IsoImport importer;private ExecutorService worker;private TextView status;private Button select,resume,start;private volatile boolean importing;private CheckBox portuguese,loadingCache;
 private static AndroidReports applicationReports;
 private AndroidReports reports;private ReportExportController export;
 @Override public void onCreate(Bundle state){super.onCreate(state);importer=new IsoImport(this);worker=Executors.newSingleThreadExecutor();
  LinearLayout layout=new LinearLayout(this);layout.setOrientation(LinearLayout.VERTICAL);layout.setPadding(24,24,24,24);
  TextView title=new TextView(this);title.setText("Urban Chaos — teste experimental");title.setTextSize(24);layout.addView(title);
  TextView note=new TextView(this);note.setText("Importe sua ISO Xbox. Este build contém o runtime recompilado; missões, fidelidade e compatibilidade no tablet ainda estão em validação. Recursos do jogo não acompanham o APK.");layout.addView(note);
  status=new TextView(this);layout.addView(status);
  select=button(layout,"Selecionar ISO",()->{Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT).addCategory(Intent.CATEGORY_OPENABLE).setType("*/*").addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION|Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);startActivityForResult(intent,71);});
  resume=button(layout,"Retomar importação",()->copy(null));
  button(layout,"Pausar importação",()->importer.cancelled=true);
  button(layout,"Descartar importação pendente",()->new android.app.AlertDialog.Builder(this).setMessage("Descartar somente a cópia incompleta? A ISO de origem, instalação ativa e saves serão preservados.").setNegativeButton("Manter",null).setPositiveButton("Descartar",(dialog,which)->{importer.cancelled=true;worker.execute(()->{importer.discard(message->runOnUiThread(()->status.setText(message)));runOnUiThread(this::refresh);});}).show());
  if(NativeSubtitles.available(this)){portuguese=new CheckBox(this);portuguese.setText("Português: menus e abertura de Hope Street (parcial)");portuguese.setChecked(getPreferences(0).getBoolean("ptbr",false));portuguese.setOnCheckedChangeListener((v,on)->getPreferences(0).edit().putBoolean("ptbr",on).apply());layout.addView(portuguese);
   loadingCache=new CheckBox(this);loadingCache.setText("Cache de carregamento experimental (usa mais espaço)");loadingCache.setChecked(getPreferences(0).getBoolean("loading_cache",false));loadingCache.setOnCheckedChangeListener((v,on)->getPreferences(0).edit().putBoolean("loading_cache",on).apply());layout.addView(loadingCache);}
  start=button(layout,"Iniciar jogo experimental",this::launch);
  try{if(applicationReports==null)applicationReports=new AndroidReports(this,BuildConfig.BUILD_ID,BuildConfig.XBE_SHA256);reports=applicationReports;importer.reports=reports.session();export=new ReportExportController(this,reports);layout.addView(export.createButton());}catch(Exception error){status.setText("Relatórios indisponíveis (UC-LOG-001)");}
  setContentView(layout);refresh();
 }
 private void launch(){
  File file=importer.selected();if(file==null||importing)return;boolean pt=portuguese!=null&&portuguese.isChecked(),cache=loadingCache!=null&&loadingCache.isChecked();importing=true;refresh();status.setText(pt||cache?"Preparando recursos nativos…":"Iniciando…");
  worker.execute(()->{long started=android.os.SystemClock.elapsedRealtime();try{NativeGame.load();String locale=pt||cache?NativeSubtitles.prepare(this,file.getAbsolutePath(),pt,cache):null;
   if((pt||cache)&&reports!=null)reports.session().event(ReportSession.Event.LOCALE_PREPARED,ReportSession.detail("duration_ms",android.os.SystemClock.elapsedRealtime()-started));
   runOnUiThread(()->{importing=false;refresh();if(!isFinishing()&&!isDestroyed())startActivity(new Intent(this,GameActivity.class).putExtra("game_path",file.getAbsolutePath()).putExtra("locale_path",locale));});
  }catch(Exception failure){if(reports!=null)reports.session().event(ReportSession.Event.LOCALE_FAILED,ReportSession.detail("exception_class",failure.getClass().getName()));runOnUiThread(()->{importing=false;refresh();status.setText("UC-LOCALE-001: não foi possível preparar os recursos opcionais. Desative Português e o cache para iniciar com os recursos originais.");});}});
 }
 private Button button(LinearLayout layout,String text,Runnable action){Button button=new Button(this);button.setText(text);button.setOnClickListener(view->action.run());layout.addView(button);return button;}
 private void refresh(){select.setEnabled(!importing);select.setText(importer.journal.exists()?"Selecionar a mesma ISO para retomar":"Selecionar ISO");resume.setEnabled(!importing&&importer.journal.exists());start.setEnabled(!importing&&importer.selected()!=null);}
 private void copy(android.net.Uri uri){if(importing)return;importing=true;importer.cancelled=false;refresh();worker.execute(()->{importer.run(uri,message->runOnUiThread(()->status.setText(message)));runOnUiThread(()->{importing=false;refresh();});});}
 @Override protected void onActivityResult(int request,int result,Intent data){super.onActivityResult(request,result,data);if(export!=null&&export.handleResult(request,result,data))return;
  if(request==71&&result==RESULT_OK&&data!=null&&data.getData()!=null){try{getContentResolver().takePersistableUriPermission(data.getData(),Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(SecurityException ignored){}copy(data.getData());}}
 @Override protected void onResume(){super.onResume();if(start!=null)refresh();}
 @Override protected void onDestroy(){importer.cancelled=true;worker.shutdown();if(export!=null)export.close();super.onDestroy();}
}
