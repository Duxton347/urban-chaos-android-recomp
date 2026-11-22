package org.urbanchaos.launcher;

import android.app.*;
import android.content.*;
import android.graphics.*;
import android.util.AtomicFile;
import android.view.*;
import android.widget.*;
import org.json.*;
import org.urbanchaos.launcher.controls.TouchLayout;
import org.urbanchaos.launcher.controls.SwipeLook;
import org.urbanchaos.launcher.controls.TouchLayout.Control;
import org.urbanchaos.launcher.diagnostics.ReportSession;
import java.io.*;
import java.nio.charset.StandardCharsets;

/** Multitouch console overlay. The original game remains the only rendered game. */
final class TouchHudView extends View {
 private final Activity activity;private final Runnable export;private final ReportSession reports;
 private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);private final RectF area=new RectF();
 private final TouchLayout.Router router=new TouchLayout.Router(new TouchLayout.Router.Sink(){
  public void button(int button,boolean down){NativeGame.padButton(button,down);}
  public void stick(int stick,float x,float y,boolean active){NativeGame.padStick(stick,x,y,active);}
 });
 private TouchLayout layout,snapshot;private Control selected;private int drag=-1;
 private boolean tablet,full=true,editing,hidden,swipe=true,lookLeft=true;
 private final SwipeLook look=new SwipeLook((x,y,active)->NativeGame.padStick(1,x,y,active));
 private final Runnable settleLook=new Runnable(){public void run(){look.tick(android.os.SystemClock.uptimeMillis());invalidate();}};private int display;
 private float dragX,dragY,originX,originY;private int insetLeft,insetRight,insetTop,insetBottom;
 private final float density;
 TouchHudView(Activity activity,Runnable export,ReportSession reports){
  super(activity);this.activity=activity;this.export=export;this.reports=reports;density=getResources().getDisplayMetrics().density;
  tablet=getResources().getConfiguration().smallestScreenWidthDp>=600;
  JSONObject settings=read("settings.json");
  if(settings!=null){tablet=settings.optBoolean("tablet",tablet);full=settings.optBoolean("combatControls7",false)?settings.optBoolean("full",true):true;swipe=settings.optBoolean("swipe",true);lookLeft=settings.optBoolean("lookLeft",true);display=Math.max(0,Math.min(2,settings.optInt("display",0)));}
  loadLayout();settings();NativeGame.displayMode(display);setFocusable(true);profileChanged("loaded");
  setContentDescription("Controles Xbox. Menu de ajustes no canto superior direito.");
  setOnApplyWindowInsetsListener((view,insets)->{
   if(android.os.Build.VERSION.SDK_INT>=30){android.graphics.Insets safe=insets.getInsets(WindowInsets.Type.systemBars()|WindowInsets.Type.displayCutout());
    insetLeft=safe.left;insetRight=safe.right;insetTop=safe.top;insetBottom=safe.bottom;
   }else{insetLeft=insets.getSystemWindowInsetLeft();insetRight=insets.getSystemWindowInsetRight();insetTop=insets.getSystemWindowInsetTop();insetBottom=insets.getSystemWindowInsetBottom();}
   confine();invalidate();return insets;
  });
 }
 private float dp(float n){return n*density;}
 private void bounds(){area.set(insetLeft+dp(6),insetTop+dp(6),Math.max(insetLeft+dp(7),getWidth()-insetRight-dp(6)),Math.max(insetTop+dp(7),getHeight()-insetBottom-dp(6)));}
 private String filename(){return (tablet?"tablet":"phone")+(full?"-xbox":"-menu")+".json";}
 private AtomicFile file(String name){return new AtomicFile(new File(new File(activity.getFilesDir(),"config/hud"),name));}
 private JSONObject read(String name){
  try(InputStream input=file(name).openRead();ByteArrayOutputStream bytes=new ByteArrayOutputStream()){
   byte[] block=new byte[1024];int count;
   while((count=input.read(block))!=-1){if(bytes.size()+count>65536)throw new IOException("HUD limit");bytes.write(block,0,count);}
   JSONObject data=new JSONObject(new String(bytes.toByteArray(),StandardCharsets.UTF_8));
   if(data.getInt("schema")!=1)throw new IOException("HUD schema");return data;
  }catch(FileNotFoundException absent){return null;}catch(Exception invalid){notice("Configuração do HUD inválida; usando padrão.");return null;}
 }
 private boolean write(String name,JSONObject data){
  AtomicFile file=file(name);FileOutputStream output=null;
  try{
   File parent=file.getBaseFile().getParentFile();if(!parent.isDirectory()&&!parent.mkdirs())throw new IOException("HUD directory");
   output=file.startWrite();output.write(data.toString().getBytes(StandardCharsets.UTF_8));file.finishWrite(output);return true;
  }catch(Exception failure){if(output!=null)file.failWrite(output);notice("Não foi possível salvar o HUD.");return false;}
 }
 private void profileChanged(String reason){if(reports!=null)reports.event(ReportSession.Event.INPUT_PROFILE,ReportSession.detail("profile",tablet?"tablet":"phone","mode",full?"xbox":"menu","state","display_"+display,"reason",reason));}
 private void settings(){try{write("settings.json",new JSONObject().put("schema",1).put("tablet",tablet).put("full",full).put("combatControls7",true).put("swipe",swipe).put("lookLeft",lookLeft).put("display",display));profileChanged("settings");}catch(JSONException impossible){throw new IllegalStateException(impossible);}}
 private void loadLayout(){
  layout=TouchLayout.defaults(tablet,full);JSONObject data=read(filename());if(data==null){confine();return;}
  try{
   TouchLayout parsed=layout.copy();JSONArray controls=data.getJSONArray("controls");java.util.HashSet<String> ids=new java.util.HashSet<>();
   boolean legacy=!full&&controls.length()==9;
   if(!legacy&&controls.length()!=parsed.controls.size())throw new JSONException("HUD count");
   for(int n=0;n<controls.length();n++){
    JSONObject item=controls.getJSONObject(n);String id=item.getString("id");Control c=parsed.find(id);
    if(!ids.add(id))throw new JSONException("HUD id");
    if(c==null&&legacy&&java.util.Arrays.asList("up","down","left","right").contains(id))continue;
    if(c==null)throw new JSONException("HUD id");
    c.configure((float)item.getDouble("x"),(float)item.getDouble("y"),(float)item.getDouble("radius"),(float)item.getDouble("alpha"));
   }
   if(legacy&&!ids.equals(new java.util.HashSet<>(java.util.Arrays.asList("a","b","x","start","back","up","down","left","right"))))throw new JSONException("HUD legacy ids");
   layout=parsed;
  }catch(Exception invalid){notice("Layout inválido; posições padrão restauradas.");}
  confine();
 }
 private boolean saveLayout(){
  try{
   JSONArray controls=new JSONArray();for(Control c:layout.controls)controls.put(new JSONObject().put("id",c.id).put("x",c.x).put("y",c.y).put("radius",c.radius).put("alpha",c.alpha));
   boolean saved=write(filename(),new JSONObject().put("schema",1).put("controls",controls));profileChanged(saved?"layout_saved":"save_failed");return saved;
  }catch(JSONException invalid){notice("Não foi possível salvar o layout.");return false;}
 }
 void releaseAll(){look.clear();removeCallbacks(settleLook);router.clear();NativeGame.clearTouch();drag=-1;invalidate();}
 @Override protected void onDetachedFromWindow(){releaseAll();super.onDetachedFromWindow();}
 @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){releaseAll();bounds();if(layout!=null)for(Control c:layout.controls)clamp(c);super.onSizeChanged(w,h,oldw,oldh);}
 private void color(int color,float alpha,Paint.Style style){paint.setColor(color);paint.setAlpha(Math.round(255*alpha));paint.setStyle(style);paint.setStrokeWidth(dp(1.5f));}
 private float gearX(){return getWidth()-insetRight-dp(30);}
 private float gearY(){return insetTop+dp(30);}
 @Override protected void onDraw(Canvas canvas){
  super.onDraw(canvas);bounds();
  if(!hidden||editing)for(Control c:layout.controls){
   if(swipe&&full&&!editing&&c.id.equals("look"))continue;
   float x=area.left+c.x*area.width(),y=area.top+c.y*area.height(),r=TouchLayout.radius(c,area.width(),area.height());boolean down=router.pressed(c);
   color(Color.rgb(12,21,29),down?.88f:c.alpha*.72f,Paint.Style.FILL);canvas.drawCircle(x,y,r,paint);
   int accent=c.id.equals("a")?0xff8bd464:c.id.equals("b")?0xfff27c79:c.id.equals("x")?0xff78b9ef:c.id.equals("y")?0xffead56d:Color.WHITE;
   color(editing&&selected==c?0xff69e0df:accent,down?1:c.alpha,Paint.Style.STROKE);canvas.drawCircle(x,y,r-dp(1),paint);
   if(c.stick>=0){
    float ax=router.axis(c.stick*2),ay=router.axis(c.stick*2+1);
    color(Color.WHITE,down?.85f:c.alpha*.55f,Paint.Style.FILL);canvas.drawCircle(x+ax*r*.55f,y-ay*r*.55f,r*.36f,paint);
   }else if(c.button>=8&&c.button<=11){
    canvas.save();canvas.translate(x,y);canvas.rotate(c.button==8?0:c.button==9?180:c.button==10?270:90);
    Path path=new Path();path.moveTo(0,-r*.32f);path.lineTo(r*.3f,r*.22f);path.lineTo(-r*.3f,r*.22f);path.close();
    color(Color.WHITE,down?1:c.alpha,Paint.Style.FILL);canvas.drawPath(path,paint);canvas.restore();
   }else{
    color(accent,down?1:c.alpha,Paint.Style.FILL);paint.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));paint.setTextAlign(Paint.Align.CENTER);
    paint.setTextSize(r*(c.label.length()>2?.40f:.78f));canvas.drawText(c.label,x,y-(paint.ascent()+paint.descent())/2,paint);
   }
  }
  if(full&&swipe&&!hidden&&!editing){color(Color.WHITE,.35f,Paint.Style.FILL);paint.setTextAlign(Paint.Align.CENTER);paint.setTextSize(dp(11));canvas.drawText("ARRASTE PARA MIRAR",getWidth()*(lookLeft?.27f:.64f),insetTop+dp(100),paint);}
  color(0xff0c151d,.83f,Paint.Style.FILL);canvas.drawCircle(gearX(),gearY(),dp(21),paint);
  color(Color.WHITE,.9f,Paint.Style.FILL);for(int n=-1;n<=1;n++)canvas.drawCircle(gearX()+dp(n*6),gearY(),dp(1.8f),paint);
  if(editing){color(Color.BLACK,.7f,Paint.Style.FILL);canvas.drawRoundRect(dp(12),insetTop+dp(10),getWidth()-insetRight-dp(65),insetTop+dp(46),dp(8),dp(8),paint);
   color(Color.WHITE,1,Paint.Style.FILL);paint.setTextAlign(Paint.Align.LEFT);paint.setTextSize(dp(13));
   canvas.drawText("EDITAR HUD · arraste · tamanho e salvar no menu ⋯",dp(22),insetTop+dp(34),paint);}
 }
 private void clamp(Control c){
  float r=TouchLayout.radius(c,area.width(),area.height());
  float mx=Math.min(.49f,r/area.width()),my=Math.min(.49f,r/area.height());
  c.x=Math.max(mx,Math.min(1-mx,c.x));c.y=Math.max(Math.min(.49f,my+dp(48)/area.height()),Math.min(1-my,c.y));
 }
 private void confine(){if(getWidth()>0&&getHeight()>0&&layout!=null){bounds();for(Control c:layout.controls)clamp(c);}}
 @Override public boolean onTouchEvent(MotionEvent event){
  bounds();int action=event.getActionMasked(),at=event.getActionIndex(),id=event.getPointerId(at);
  float x=event.getX(at),y=event.getY(at);
  if(action==MotionEvent.ACTION_DOWN||action==MotionEvent.ACTION_POINTER_DOWN){
   if(Math.hypot(x-gearX(),y-gearY())<=dp(25)){releaseAll();showMenu();return true;}
   if(!hidden||editing){Control c=layout.hit(x-area.left,y-area.top,area.width(),area.height());
    if(swipe&&full&&!editing&&c!=null&&c.id.equals("look"))c=null;
    if(editing&&drag<0){selected=c;if(c!=null){drag=id;dragX=x;dragY=y;originX=c.x;originY=c.y;}}
    else if(!editing){
     if(c!=null)router.down(id,c,x-area.left,y-area.top,area.width(),area.height());
     else if(full&&swipe&&((x<getWidth()*.5f)==lookLeft))look.down(id,x,y);
    }
   }
  }else if(action==MotionEvent.ACTION_MOVE){
   for(int n=0;n<event.getPointerCount();n++){
    int pointer=event.getPointerId(n);
    if(editing&&pointer==drag&&selected!=null){selected.x=originX+(event.getX(n)-dragX)/area.width();selected.y=originY+(event.getY(n)-dragY)/area.height();clamp(selected);}
    else if(!editing){
     if(look.owns(pointer)){look.move(pointer,event.getX(n),event.getY(n),Math.min(area.width(),area.height()),android.os.SystemClock.uptimeMillis());removeCallbacks(settleLook);postDelayed(settleLook,90);}
     else router.move(pointer,event.getX(n)-area.left,event.getY(n)-area.top,area.width(),area.height());
    }
   }
  }else if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_POINTER_UP){look.up(id);router.up(id);if(id==drag)drag=-1;}
  else if(action==MotionEvent.ACTION_CANCEL)releaseAll();
  invalidate();return true;
 }
 private void notice(String message){Toast.makeText(activity,message,Toast.LENGTH_SHORT).show();}
 private void help(){new AlertDialog.Builder(activity).setTitle("Como usar").setMessage("Menus: o joystick esquerdo navega, A seleciona, B volta e X apaga um perfil quando a tela indicar. START abre a pausa.\n\nMira por arrasto: deslize na área livre à esquerda (troque o lado em ⋯ → Mira). Não precisa tocar em um joystick direito. Ao parar o dedo, a mira para. Os botões têm prioridade sobre a área de mira.\n\nControle Xbox completo é o padrão: L move; LT/RT são gatilhos; L3/R3 pressionam os analógicos. Os nomes seguem o controle original. O esquema escolhido dentro do jogo determina cada ação.\n\nUse vários dedos e mantenha pressionado para segurar uma ação. O menu ⋯ ajusta a tela e o HUD. O editor não envia comandos ao jogo. Pause com START antes de editar durante uma missão.\n\nA sensibilidade e a inversão da mira ficam no próprio jogo: Options → Controller → Horizontal/Vertical Sensitivity. O desempenho e os defeitos gráficos da missão continuam em investigação.").setPositiveButton("Entendi",null).show();}
 private void showMenu(){
  String[] items=editing?new String[]{"Salvar layout","Cancelar edição","Tamanho do controle selecionado","Opacidade do controle selecionado","Restaurar posições padrão"}:
   new String[]{"Enquadramento da tela","Modo de controle","Editar HUD","Perfil: "+(tablet?"tablet":"celular"),hidden?"Mostrar controles":"Ocultar controles","Como usar","Exportar relatório","Mira: "+(swipe?(lookLeft?"arrasto à esquerda":"arrasto à direita"):"joystick direito")};
  new AlertDialog.Builder(activity).setTitle(editing?"Editar HUD":"Tela e controles").setItems(items,(dialog,which)->{
   if(editing){switch(which){
    case 0:if(saveLayout()){editing=false;snapshot=null;selected=null;}break;
    case 1:layout=snapshot;editing=false;snapshot=null;selected=null;break;
    case 2:case 3:adjust(which==2);break;
    case 4:layout=TouchLayout.defaults(tablet,full);confine();selected=null;break;
   }}else switch(which){
    case 0:displayOptions();break;
    case 1:new AlertDialog.Builder(activity).setTitle("Modo de controle").setSingleChoiceItems(new String[]{"Navegação de menus","Controle Xbox completo"},full?1:0,(d,n)->{releaseAll();full=n==1;loadLayout();settings();d.dismiss();invalidate();}).show();break;
    case 2:releaseAll();snapshot=layout.copy();editing=true;hidden=false;selected=null;notice("Arraste um controle; use ⋯ para tamanho, opacidade e salvar.");break;
    case 3:new AlertDialog.Builder(activity).setTitle("Perfil independente").setSingleChoiceItems(new String[]{"Celular","Tablet"},tablet?1:0,(d,n)->{releaseAll();tablet=n==1;loadLayout();settings();d.dismiss();invalidate();}).show();break;
    case 4:releaseAll();hidden=!hidden;break;
    case 5:help();break;
    case 6:if(export!=null)export.run();break;
    case 7:new AlertDialog.Builder(activity).setTitle("Mira por toque").setSingleChoiceItems(new String[]{"Arrastar área livre à esquerda","Arrastar área livre à direita","Joystick direito"},swipe?(lookLeft?0:1):2,(d,n)->{releaseAll();swipe=n<2;lookLeft=n==0;settings();d.dismiss();invalidate();}).show();break;
   }
   invalidate();
  }).setNegativeButton("Fechar",null).show();
 }
 private void displayOptions(){
  new AlertDialog.Builder(activity).setTitle("Enquadramento da tela").setSingleChoiceItems(new String[]{"Ajustar — imagem inteira, proporção original","Preencher — ocupa a tela e recorta bordas","Esticar — ocupa a tela e altera proporções"},display,(dialog,n)->{display=n;NativeGame.displayMode(n);settings();dialog.dismiss();}).setNegativeButton("Cancelar",null).show();
 }
 private void adjust(boolean size){
  if(selected==null){notice("Selecione um controle no editor primeiro.");return;}
  Control c=selected;SeekBar seek=new SeekBar(activity);seek.setMax(size?175:85);seek.setProgress(Math.round(size?c.radius*1000-25:c.alpha*100-15));
  seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
   public void onProgressChanged(SeekBar bar,int value,boolean user){if(user){if(size){c.radius=(value+25)/1000f;clamp(c);}else c.alpha=(value+15)/100f;invalidate();}}
   public void onStartTrackingTouch(SeekBar bar){}public void onStopTrackingTouch(SeekBar bar){}
  });
  new AlertDialog.Builder(activity).setTitle(c.label+(size?" — tamanho":" — opacidade")).setView(seek).setPositiveButton("OK",null).show();
 }
}
