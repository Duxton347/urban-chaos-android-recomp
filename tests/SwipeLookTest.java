import org.urbanchaos.launcher.controls.SwipeLook;
public class SwipeLookTest {
 public static void main(String[] args){
  float[] a=new float[3];SwipeLook s=new SwipeLook((x,y,on)->{a[0]=x;a[1]=y;a[2]=on?1:0;});
  assert s.down(7,100,100);assert !s.down(8,0,0);
  s.move(8,400,0,800,100);assert a[0]==0;
  s.move(7,136,64,800,100);assert a[0]>.7f&&a[1]>.7f&&a[0]<=1;
  s.tick(189);assert a[0]>.7f;s.tick(190);assert a[0]==0&&a[1]==0;
  s.up(8);assert s.owns(7);s.up(7);assert a[2]==0&&!s.owns(7);
  assert s.down(9,0,0);s.move(9,-36,36,800,200);assert a[0]<0&&a[1]<0;
  s.clear();assert a[2]==0;
  System.out.println("Swipe look ownership, saturation, direction, idle stop and cancel passed");
 }
}
