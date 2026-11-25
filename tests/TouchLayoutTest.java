import org.urbanchaos.launcher.controls.TouchLayout;
import org.urbanchaos.launcher.controls.TouchLayout.Control;
import java.util.*;
public final class TouchLayoutTest {
 static int checks;
 static void check(boolean ok){checks++;if(!ok)throw new AssertionError("HUD check "+checks);}
 public static void main(String[] args){
  List<String> events=new ArrayList<>();float[] axes=new float[4];
  TouchLayout.Router router=new TouchLayout.Router(new TouchLayout.Router.Sink(){
   public void button(int b,boolean d){events.add(b+":"+d);}
   public void stick(int s,float x,float y,boolean a){axes[s*2]=x;axes[s*2+1]=y;events.add("s"+s+":"+a);}
  });
  TouchLayout menu=TouchLayout.defaults(true,false),xbox=TouchLayout.defaults(true,true);
  check(menu.find("a").button==0&&menu.find("move").stick==0&&menu.find("up")==null&&menu.controls.size()==6&&menu.find("start").button==12);
  check(xbox.find("rt").button==7&&xbox.find("look").stick==1);
  check(xbox.find("up").button==8&&xbox.find("move").stick==0);
  Control a=menu.find("a"),b=menu.find("b");
  router.down(21,a,0,0,1280,800);router.down(37,b,0,0,1280,800);
  check(events.equals(Arrays.asList("0:true","1:true")));
  router.up(21);check(!router.pressed(a)&&router.pressed(b));
  router.down(49,b,0,0,1280,800);router.up(37);check(router.pressed(b));router.up(49);check(!router.pressed(b));
  Control left=xbox.find("move"),right=xbox.find("look");
  check(router.down(8,left,left.x*1280,left.y*800,1280,800));
  check(!router.down(9,left,left.x*1280,left.y*800,1280,800));
  router.down(19,right,right.x*1280,right.y*800,1280,800);router.down(23,xbox.find("rt"),0,0,1280,800);
  router.move(8,left.x*1280+10000,left.y*800,1280,800);check(Math.abs(axes[0]-1)<.0001&&axes[1]==0);
  router.move(19,right.x*1280,right.y*800-10000,1280,800);check(Math.abs(axes[3]-1)<.0001);
  router.up(8);check(!router.pressed(left)&&router.pressed(right)&&router.pressed(xbox.find("rt")));
  router.clear();check(!router.pressed(right)&&!router.pressed(xbox.find("rt"))&&axes[3]==0);
  check(menu.hit(a.x*1280,a.y*800,1280,800)==a);
  TouchLayout saved=menu.copy();a.configure(.6f,.7f,.08f,.8f);check(saved.find("a").x!=a.x);
  try{a.configure(Float.NaN,.5f,.1f,.5f);throw new AssertionError();}catch(IllegalArgumentException good){checks++;}
  check(TouchLayout.defaults(false,false).find("a").radius>saved.find("a").radius);
  TouchLayout compact=TouchLayout.mobile(true);
  check(compact.find("y")!=null&&compact.find("rt")!=null);
  Control aux=compact.find("black");check(compact.hit(aux.x*1280,aux.y*800,1280,800,c->!TouchLayout.extra(c.id))!=aux);
  check(TouchLayout.extra("black")&&!TouchLayout.extra("a"));
  router.toggleMode(6,true);router.toggleMode(14,true);router.commandWheel(true);
  events.clear();Control shield=xbox.find("lt");
  router.down(100,shield,0,0,1280,800);router.up(100);check(router.pressed(shield)&&events.get(events.size()-1).equals("6:true"));
  router.down(101,shield,0,0,1280,800);router.down(102,shield,0,0,1280,800);router.up(101);router.up(102);check(!router.pressed(shield)&&events.get(events.size()-1).equals("6:false"));
  router.down(103,xbox.find("ls"),0,0,1280,800);router.up(103);check(router.pressed(xbox.find("ls")));router.clear();check(!router.pressed(xbox.find("ls")));
  Control orders=xbox.find("down");float ox=orders.x*1280,oy=orders.y*800;
  router.down(104,orders,ox,oy,1280,800);check(router.pressed(orders));check(!router.down(105,orders,ox,oy,1280,800));
  router.move(104,ox+200,oy,1280,800);check(router.pressed(xbox.find("right"))&&router.pressed(orders));
  router.down(106,xbox.find("rt"),0,0,1280,800);router.up(104);check(!router.pressed(xbox.find("right"))&&router.pressed(xbox.find("rt")));router.clear();check(!router.pressed(xbox.find("rt")));
  events.clear();router.down(110,orders,ox+30,oy,1280,800);router.move(110,ox+30,oy,1280,800);check(events.equals(Arrays.asList("9:true")));
  router.move(110,ox+30,oy-200,1280,800);check(router.pressed(xbox.find("up"))&&router.pressed(orders));
  router.move(110,ox-200,oy,1280,800);check(router.pressed(xbox.find("left"))&&!router.pressed(xbox.find("up")));
  router.up(110);check(!router.pressed(orders)&&!router.pressed(xbox.find("left")));router.up(110);router.clear();
  System.out.println("HUD layout/multitouch checks passed: "+checks+"; no game execution");
 }
}
