#include "../../src/machine/vf2_impact_sound.h"
#include "../../src/host/fight_haptics.h"
#include <cassert>
#include <cmath>
int main() {
 vf2_impact_sound s;
 s.voice(10,1);assert(!s.take(10000).count[0]); // sound alone is silent
 for(unsigned note:{1u,3u,0x25u,0x24u,2u,0x1eu,0x2au,0xffu}) {
  s.clear();s.contact(100,0,true);s.voice(1400,note);
  assert(!s.take(8100).count[0]); // inclusive window, then end
  auto b=s.take(8101);assert(b.count[0]==1 && b.estimate[0]==s.estimate_damage(int(note)));
  assert(!s.take(20000).count[0]);
 }
 s.clear();s.contact(100,0,true);s.voice(200,1);s.voice(300,1);
 assert(s.take(9000).estimate[0]==0); // duplicated slots are ambiguous
 s.clear();s.contact(100,0,true);s.contact(110,1,false);s.voice(200,0x25);
 auto b=s.take(9000);assert(b.count[0]==1 && b.estimate[0]==0 && b.count[1]==0);
 s.clear();s.contact(100,1,true);s.voice(99,0x24);s.voice(200,0x22);
 b=s.take(9000);assert(b.count[1]==1 && b.estimate[1]==0);
 s.clear();s.contact(100,0,true);s.clear();assert(!s.take(9000).count[0]);
 s.contact(100,0,true);s.voice(10000,1);b=s.take(10001);assert(b.count[0]==1 && b.estimate[0]==0);
 s.clear();for(int i=0;i<20;++i)s.contact(100,0,true);
 b=s.take(9000);assert(b.count[0]==20 && b.estimate[0]==0); // overflow emits fallback, never loses contacts
 using namespace fight_haptics;
 auto pulse=[](int estimate,int damage=0){
  Tracker t;Snapshot a;a.live=true;a.maximum=220;a.health[0]=100;a.health[1]=0;a.damage_events=true;
  t.advance(a,3,75);a.frame=1;a.contact_total[1]=1;a.contact_estimate[1]=estimate;a.damage_total[1]=damage;
  return t.advance(a,3,75);
 };
 auto light=pulse(15),medium=pulse(22),heavy=pulse(45),fallback=pulse(0);
 assert(light[0].high<medium[0].high && medium[0].high<heavy[0].high);
 assert(light[1].low<medium[1].low && medium[1].low<heavy[1].low);
 assert(heavy[0].high==fallback[0].high && heavy[1].ms==fallback[1].ms);
 assert(!light[0].knockout && !heavy[1].knockout);
 auto measured=pulse(45,12),same=pulse(0,12);assert(measured[0].high==same[0].high);
 Tracker t;Snapshot a;a.live=true;a.maximum=220;a.health[0]=100;a.health[1]=0;a.damage_events=true;
 t.advance(a,3,75);a.frame=1;a.contact_estimate[1]=45;auto none=t.advance(a,3,75);assert(none[0].ms==0 && none[1].ms==0);
}
