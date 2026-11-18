package org.urbanchaos.launcher;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.Gravity;
import android.widget.*;
import org.libsdl.app.SDLActivity;
import org.urbanchaos.launcher.diagnostics.*;
/** Actual SDL runtime Activity; the initial overlay exposes only verified Xbox menu inputs. */
public final class GameActivity extends SDLActivity {
 private AndroidReports reports;private ReportExportController export;
 @Override protected String[] getArguments(){String path=getIntent().getStringExtra("game_path");return path==null?new String[0]:new String[]{path};}
 @Override protected void onCreate(Bundle state){super.onCreate(state);if(mBrokenLibraries)return;
  try{reports=new AndroidReports(this,BuildConfig.BUILD_ID,BuildConfig.XBE_SHA256);NativeReports.attach(reports.session(),BuildConfig.BUILD_ID,BuildConfig.XBE_SHA256,false);export=new ReportExportController(this,reports);}catch(Exception ignored){}
  LinearLayout buttons=new LinearLayout(this);buttons.setGravity(Gravity.BOTTOM|Gravity.END);
  int height=(int)(56*getResources().getDisplayMetrics().density);
  String[] names={"A","B","START","↑","↓","←","→"};
  for(int n=0;n<names.length;n++){final int key=n;Button button=new Button(this);button.setText(names[n]);button.setAlpha(.7f);button.setOnTouchListener((view,event)->{int action=event.getActionMasked();if(action==MotionEvent.ACTION_DOWN){NativeGame.menuButton(key,true);view.setPressed(true);}else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){NativeGame.menuButton(key,false);view.setPressed(false);}return true;});buttons.addView(button,new LinearLayout.LayoutParams(0,height,1));}
  if(export!=null)buttons.addView(export.createButton(),new LinearLayout.LayoutParams(0,height,2));
  android.widget.RelativeLayout.LayoutParams params=new android.widget.RelativeLayout.LayoutParams(-1,-2);params.addRule(android.widget.RelativeLayout.ALIGN_PARENT_BOTTOM);mLayout.addView(buttons,params);
 }
 @Override protected void onPause(){if(!mBrokenLibraries)for(int n=0;n<7;n++)NativeGame.menuButton(n,false);if(reports!=null)reports.lifecycle("pause");super.onPause();}
 @Override protected void onResume(){super.onResume();if(reports!=null)reports.lifecycle("resume");}
 @Override protected void onActivityResult(int request,int result,android.content.Intent data){if(export==null||!export.handleResult(request,result,data))super.onActivityResult(request,result,data);}
 @Override protected void onDestroy(){if(export!=null)export.close();super.onDestroy();}
}
