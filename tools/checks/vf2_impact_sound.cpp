#include "../../src/machine/vf2_impact_sound.h"
#include "../../src/host/fight_haptics_vf2.h"
#include <cassert>
#include <cstdio>

int main() {
 vf2_impact_sound s;
 s.voice(10,1);assert(!s.take(10000).count[0]); // sound alone is silent
 for(unsigned note:{1u,3u,0x25u,0x24u,2u,0x1eu,0x2au,0xffu}) {
  s.clear();s.contact(100,0,true,true,3);s.voice(1400,note);
  assert(!s.take(8100).count[0]);
  auto b=s.take(8101);assert(b.count[0]==1 && b.estimate[0]==s.estimate_damage(int(note)));
  assert(!s.take(20000).count[0]);
 }
 s.clear();s.contact(100,0,true);s.voice(200,1);s.voice(300,1);
 assert(s.take(9000).estimate[0]==0); // multiple slots are ambiguous
 s.clear();s.contact(100,0,true);s.contact(110,1,false);s.voice(200,0x25);
 auto b=s.take(9000);assert(b.count[0]==1 && b.estimate[0]==0 && b.count[1]==0);
 s.clear();s.contact(100,1,true);s.voice(99,0x24);s.voice(200,0x22);
 b=s.take(9000);assert(b.count[1]==1 && b.estimate[1]==0);
 s.clear();s.contact(100,0,true);s.clear();assert(!s.take(9000).count[0]);
 s.contact(100,0,true);s.voice(10000,1);b=s.take(10001);assert(b.count[0]==1 && b.estimate[0]==0);
 s.clear();for(int i=0;i<20;++i)s.contact(100,0,true);
 b=s.take(9000);assert(b.count[0]==20 && b.estimate[0]==0);
 using namespace fight_haptics;
 // Distinct inferred strengths for both victims, all supported health ceilings/gains.
 for(int maximum:{176,200,220,240}) for(int gain:{0,25,75,100}) for(int victim=0;victim<2;++victim) {
  auto impact=[&](unsigned note,bool lethal) {
   vf2_impact_sound matcher;Tracker tracker;
   Snapshot a;a.live=true;a.maximum=maximum;a.health[0]=a.health[1]=maximum;
   a.health[victim]=lethal?3:0;a.damage_events=true;a.knockout_events=true;
   tracker.advance(a,3,gain);
   matcher.contact(16000,victim,true,lethal,lethal?3:0);
   a.frame=1;a.health[victim]=0;
   assert(!tracker.advance(a,3,gain)[0].ms); // no early clipped KO or duplicate pulse
   assert(!matcher.take(17000).count[victim]); // contact crosses original-frame boundary
   matcher.voice(19000,note);
   auto batch=matcher.take(24001);
   a.frame=2;a.contact_total[victim]+=batch.count[victim];
   a.damage_total[victim]+=batch.damage[victim];a.knockout_total[victim]+=batch.knockouts[victim];
   a.contact_estimate[victim]=batch.estimate[victim];
   auto result=tracker.advance(a,3,gain);
   assert(result[0].ms && result[1].ms);
   assert(result[0].knockout==lethal && result[1].knockout==lethal);
   assert(!tracker.advance(a,3,gain)[0].ms); // duplicate display frame
   a.frame=3;assert(!tracker.advance(a,3,gain)[0].ms); // stale estimate is not an event
   return result;
  };
  for(bool lethal:{false,true}) {
   auto light=impact(1,lethal),medium=impact(0x25,lethal),heavy=impact(0x24,lethal);
   if(gain && lethal) for(int p=0;p<2;++p) {
    assert(light[p].low<medium[p].low && medium[p].low<heavy[p].low);
    assert(light[p].high<medium[p].high && medium[p].high<heavy[p].high);
    assert(light[p].ms<medium[p].ms && medium[p].ms<heavy[p].ms);
   }
   if(!lethal) for(unsigned note:{1u,3u,0x25u,0x24u,0xffu,0x22u,256u}) {
    auto fixed=impact(note,false);
    for(int p=0;p<2;++p) {
     assert(fixed[p].low==heavy[p].low && fixed[p].high==heavy[p].high && fixed[p].ms==heavy[p].ms);
     const bool received=p==victim;
     const float factor=gain/100.0f;
     assert(std::abs(fixed[p].low-(received?0.90f:0.52f)*factor)<0.000001f);
     assert(std::abs(fixed[p].high-(received?0.65f:0.90f)*factor)<0.000001f);
     assert(fixed[p].ms==(received?100u:65u));
    }
   }
   for(int p=0;p<2;++p)assert(heavy[p].low<=1 && heavy[p].high<=1);
   if(!gain)assert(light[0].low==0 && heavy[1].high==0);
  }
 }
 // Unknown lethal sound preserves measured remainder and the single KO event.
 s.clear();s.contact(100,0,true,true,7);s.voice(200,0xff);b=s.take(9000);
 assert(b.count[0]==1 && b.knockouts[0]==1 && b.damage[0]==7 && b.estimate[0]==0);
 s.clear();s.contact(100,1,true,true,9);s.clear();b=s.take(9000);
 assert(!b.count[1] && !b.knockouts[1] && !b.damage[1]); // pause/reset cancel the pending KO
 // Ordinary nonlethal measured hits ignore estimates and retain their previous curve.
 Tracker normal,control;Snapshot a;a.live=true;a.maximum=220;a.health[0]=a.health[1]=176;a.damage_events=true;
 normal.advance(a,3,75);control.advance(a,3,75);
 a.frame=1;a.health[1]-=12;a.damage_total[1]=12;a.contact_total[1]=1;
 auto expected=control.advance(a,3,75);a.contact_estimate[1]=45;auto actual=normal.advance(a,3,75);
 for(int p=0;p<2;++p)assert(actual[p].low==expected[p].low && actual[p].high==expected[p].high && actual[p].ms==expected[p].ms);
 // A clipped estimate cannot erase measured damage; no contact means no sound pulse.
 Tracker t;Snapshot c;c.live=true;c.maximum=220;c.health[0]=100;c.health[1]=0;c.damage_events=true;
 t.advance(c,3,75);c.frame=1;c.contact_estimate[1]=45;assert(!t.advance(c,3,75)[0].ms);
 // The production VF2 adapter carries delayed knockout counters.
 std::vector<uint8_t> work(0x30000);work[0x2b]=9;work[0x31]=13;work[0x35]=32;
 uint64_t ko[2]={1,0};auto adapted=vf2_haptic_snapshot(work,5,220,nullptr,nullptr,nullptr,ko);
 assert(adapted.knockout_events && adapted.knockout_total[0]==1 && adapted.live==false);
 puts("PASS: sound-only silence, bounded causal association, lethal estimation and fixed maximum post-KO intensity, ordinary-hit parity and routing");
}
