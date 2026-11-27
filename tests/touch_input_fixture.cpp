#include "input/touch_state.h"
#include "display/viewport.h"
#include <stdexcept>
#include <limits>
#include <iostream>
static int checks=0;
static void Check(bool value){++checks;if(!value)throw std::runtime_error("touch/display contract failed at "+std::to_string(checks));}
int main(){
 using namespace urban::touch;
 State touch;input::Pad physical;physical.connected=true;physical.rx=1234;physical.analog[RT]=64;
 // Tap entirely between two slow guest polls must still produce one down report.
 touch.Set(A,true,10);touch.Set(A,false,20);
 auto p=touch.Merge(physical,200010);Check(p.analog[A]==255&&p.analog[RT]==64);
 Check(touch.Merge(physical,200020).analog[A]==0);
 Check(touch.Snapshot().delivered==1&&touch.Snapshot().max_us==200000);
 touch.Set(RT,true,30);for(int n=0;n<50;n++)Check(touch.Merge({},200030+n).analog[RT]==255);
 touch.Set(RT,false,40);Check(touch.Merge({},200100).analog[RT]==0);
 // A second tap needs an observed release edge; no arbitrary wall-clock delay.
 touch.Set(A,true,100);Check(touch.Merge({},110).analog[A]==255);
 touch.Set(A,false,115);touch.Set(A,true,116);touch.Set(A,false,117);
 Check(touch.Merge({},120).analog[A]==0);Check(touch.Merge({},130).analog[A]==255);Check(touch.Merge({},140).analog[A]==0);
 for(int button=Up;button<Count;button++){touch.Set(button,true,200);Check(touch.Merge({},220).buttons&(1u<<(button-8)));touch.Clear();}
 touch.Axis(0,1,-1,true);touch.Axis(1,.5f,.25f,true);p=touch.Merge(physical,300);
 Check(p.lx==32767&&p.ly==-32767&&p.rx==16384&&p.ry==8192);
 touch.Axis(1,0,0,false);Check(touch.Merge(physical,310).rx==1234);
 touch.Axis(0,std::numeric_limits<float>::quiet_NaN(),0,true);Check(touch.Merge({},320).lx==0);
 touch.Set(B,true,330);touch.Clear();p=touch.Merge({},340);Check(p.analog[B]==0&&p.lx==0&&p.buttons==0);
 for(int n=0;n<10000;n++){touch.Set(X,true,n);touch.Set(X,false,n);}
 Check(touch.Merge({},20000).analog[X]==255);Check(touch.Merge({},20001).analog[X]==0);Check(touch.Merge({},20002).analog[X]==0);
 using namespace urban::display;
 auto fit=Viewport(1280,800,4.f/3,Mode::Fit),fill=Viewport(1280,800,4.f/3,Mode::Fill),stretch=Viewport(1280,800,4.f/3,Mode::Stretch);
 Check(std::abs(fit.w-1066.6667f)<.01f&&std::abs(fit.h-800)<.01f&&fit.x>106);
 Check(fill.w==1280&&std::abs(fill.h-960)<.01f&&std::abs(fill.y+80)<.01f);
 Check(stretch.x==0&&stretch.y==0&&stretch.w==1280&&stretch.h==800);
 auto phone=Viewport(2400,1080,4.f/3,Mode::Fill);Check(phone.w==2400&&std::abs(phone.h-1800)<.01f&&std::abs(phone.y+360)<.01f);
 Check(Viewport(0,800,4.f/3,Mode::Fit).w==0);
 std::cout<<"{\"touch_display_checks\":"<<checks<<",\"game_data_used\":false}\n";
}
