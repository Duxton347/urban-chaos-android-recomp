package org.urbanchaos.launcher;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import org.libsdl.app.SDLActivity;
import org.urbanchaos.launcher.diagnostics.*;
/** SDL game surface with an independent, editable Xbox-source touch overlay. */
public final class GameActivity extends SDLActivity {
 private AndroidReports reports;private ReportExportController export;private TouchHudView hud;
 @Override protected String[] getArguments(){String path=getIntent().getStringExtra("game_path");String locale=getIntent().getStringExtra("locale_path");return path==null?new String[0]:locale==null?new String[]{path}:new String[]{path,locale};}
 @Override protected void onCreate(Bundle state){super.onCreate(state);if(mBrokenLibraries)return;
  try{reports=new AndroidReports(this,BuildConfig.BUILD_ID,BuildConfig.XBE_SHA256);NativeReports.attach(reports.session(),BuildConfig.BUILD_ID,BuildConfig.XBE_SHA256,false);export=new ReportExportController(this,reports);}catch(Exception ignored){}
  if(reports!=null)reports.session().event(ReportSession.Event.LOCALE_STATE,ReportSession.detail("state",getIntent().getStringExtra("locale_path")==null?"original":"cache_requested"));
  getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
  hud=new TouchHudView(this,()->{if(export!=null)export.selectDestination();},reports==null?null:reports.session());
  mLayout.addView(hud,new RelativeLayout.LayoutParams(-1,-1));hud.requestApplyInsets();
 }
 @Override protected void onPause(){if(hud!=null)hud.releaseAll();if(reports!=null)reports.lifecycle("pause");super.onPause();}
 @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus&&hud!=null)hud.releaseAll();}
 @Override protected void onResume(){super.onResume();if(reports!=null)reports.lifecycle("resume");}
 @Override protected void onActivityResult(int request,int result,android.content.Intent data){if(export==null||!export.handleResult(request,result,data))super.onActivityResult(request,result,data);}
 @Override protected void onDestroy(){if(export!=null)export.close();super.onDestroy();}
}
