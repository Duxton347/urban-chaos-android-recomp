package org.urbanchaos.launcher.controls;

/** Relative touch motion converted to a short right-stick pulse. No title memory writes. */
public final class SwipeLook {
 public interface Sink {void axis(float x,float y,boolean active);}
 private final Sink sink;private int owner=-1;private float x,y;private long moved;
 public SwipeLook(Sink sink){this.sink=sink;}
 public boolean down(int pointer,float px,float py){if(owner!=-1)return false;owner=pointer;x=px;y=py;return true;}
 public boolean owns(int pointer){return owner==pointer;}
 public void move(int pointer,float px,float py,float extent,long now){
  if(!owns(pointer)||extent<=0)return;
  float dx=(px-x)/(extent*.045f),dy=(y-py)/(extent*.045f);x=px;y=py;
  float length=(float)Math.hypot(dx,dy);if(length>1){dx/=length;dy/=length;}
  sink.axis(dx,dy,true);moved=now;
 }
 public void tick(long now){if(owner!=-1&&now-moved>=90)sink.axis(0,0,true);}
 public void up(int pointer){if(owns(pointer))clear();}
 public void clear(){if(owner!=-1)sink.axis(0,0,false);owner=-1;}
}
