package org.urbanchaos.launcher.controls;

import java.util.*;

/** Authored console-source layout. No title action addresses or game simulation. */
public final class TouchLayout {
 public static final class Control {
  public final String id,label; public final int button,stick;
  public float x,y,radius,alpha;
  Control(String id,String label,int button,int stick,float x,float y,float radius){this.id=id;this.label=label;this.button=button;this.stick=stick;this.x=x;this.y=y;this.radius=radius;alpha=.58f;}
  public Control copy(){Control c=new Control(id,label,button,stick,x,y,radius);c.alpha=alpha;return c;}
  public void configure(float x,float y,float radius,float alpha){
   if(!Float.isFinite(x)||!Float.isFinite(y)||!Float.isFinite(radius)||!Float.isFinite(alpha)||x<0||x>1||y<0||y>1||radius<.025f||radius>.20f||alpha<.15f||alpha>1)throw new IllegalArgumentException("Invalid HUD geometry");
   this.x=x;this.y=y;this.radius=radius;this.alpha=alpha;
  }
 }
 public final List<Control> controls=new ArrayList<>();
 public static TouchLayout defaults(boolean tablet,boolean full){
  TouchLayout layout=new TouchLayout();float r=tablet?.050f:.064f;
  layout.add("a","A",0,-1,.89f,.77f,r);layout.add("b","B",1,-1,.95f,.59f,r);
  layout.add("x","X",2,-1,.80f,.61f,r);
  layout.add("start","START",12,-1,.56f,.91f,r*.8f);
  layout.add("back","BACK",13,-1,.44f,.91f,r*.8f);
  float cx=full?.39f:.14f,cy=full?.77f:.72f;
  layout.add("up","UP",8,-1,cx,cy-.12f,r);layout.add("down","DOWN",9,-1,cx,cy+.12f,r);
  layout.add("left","LEFT",10,-1,cx-.075f,cy,r);layout.add("right","RIGHT",11,-1,cx+.075f,cy,r);
  if(full){
   layout.add("y","Y",3,-1,.88f,.42f,r);
   layout.add("black","BLACK",4,-1,.72f,.39f,r*.85f);layout.add("white","WHITE",5,-1,.26f,.39f,r*.85f);
   layout.add("lt","LT",6,-1,.12f,.23f,r);layout.add("rt","RT",7,-1,.88f,.23f,r);
   layout.add("ls","L3",14,-1,.25f,.92f,r*.75f);layout.add("rs","R3",15,-1,.72f,.94f,r*.75f);
   layout.add("move","L",-1,0,.14f,.73f,.11f);layout.add("look","R",-1,1,.68f,.70f,.105f);
  }
  return layout;
 }
 private void add(String id,String label,int button,int stick,float x,float y,float radius){controls.add(new Control(id,label,button,stick,x,y,radius));}
 public TouchLayout copy(){TouchLayout other=new TouchLayout();for(Control c:controls)other.controls.add(c.copy());return other;}
 public Control find(String id){for(Control c:controls)if(c.id.equals(id))return c;return null;}
 public static float radius(Control c,float w,float h){return c.radius*Math.min(w,h);}
 public Control hit(float x,float y,float w,float h){
  Control best=null;float score=Float.MAX_VALUE;
  for(Control c:controls){float dx=x-c.x*w,dy=y-c.y*h,r=radius(c,w,h);float d=(dx*dx+dy*dy)/(r*r);
   if(d<=1.35f*1.35f&&d<score){score=d;best=c;}}
  return best;
 }
 public static final class Router {
  public interface Sink {void button(int button,boolean down);void stick(int stick,float x,float y,boolean active);}
  private final Sink sink;private final Map<Integer,Control> owners=new HashMap<>();private final int[] counts=new int[16];
  private final boolean[] sticks=new boolean[2];private final float[] axes=new float[4];
  public Router(Sink sink){this.sink=sink;}
  public boolean down(int pointer,Control c,float x,float y,float w,float h){
   if(c==null||owners.containsKey(pointer)||(c.stick>=0&&sticks[c.stick]))return false;
   owners.put(pointer,c);
   if(c.button>=0){if(counts[c.button]++==0)sink.button(c.button,true);}
   else {sticks[c.stick]=true;move(pointer,x,y,w,h);}
   return true;
  }
  public void move(int pointer,float x,float y,float w,float h){
   Control c=owners.get(pointer);if(c==null||c.stick<0)return;
   float r=radius(c,w,h),dx=(x-c.x*w)/r,dy=(c.y*h-y)/r;
   float length=(float)Math.hypot(dx,dy);
   if(length<=.08f){dx=dy=0;}else{float magnitude=(Math.min(length,1)-.08f)/.92f;dx=dx/length*magnitude;dy=dy/length*magnitude;}
   axes[c.stick*2]=dx;axes[c.stick*2+1]=dy;sink.stick(c.stick,dx,dy,true);
  }
  public void up(int pointer){
   Control c=owners.remove(pointer);if(c==null)return;
   if(c.button>=0){if(--counts[c.button]==0)sink.button(c.button,false);}
   else {sticks[c.stick]=false;axes[c.stick*2]=axes[c.stick*2+1]=0;sink.stick(c.stick,0,0,false);}
  }
  public boolean pressed(Control c){return c.button>=0?counts[c.button]>0:sticks[c.stick];}
  public float axis(int n){return axes[n];}
  public void clear(){for(int id:new ArrayList<>(owners.keySet()))up(id);}
 }
}
