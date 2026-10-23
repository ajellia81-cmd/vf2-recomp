#include "../../src/host/vf2_damage_events.h"
#include "../../src/host/fight_haptics_vf2.h"
#include <cassert>
#include <cstdio>
int main() {
 assert(vf2_contact_store(0x18a7c,true,29,0,9));
 assert(vf2_contact_store(0x18a7c,true,0,0,9));
 assert(!vf2_contact_store(0x14214,true,0,176,9));
 assert(!vf2_contact_store(0x18a7c,false,0,0,9));
 assert(!vf2_contact_store(0x18a7c,true,0,0,3));
 assert(!vf2_contact_store(0x18a7c,true,10,10,9));
 using namespace fight_haptics;
 for(int victim=0;victim<2;++victim) {
  std::vector<uint8_t> w(0x30000);w[0x2b]=9;w[0x31]=9;w[0x35]=2;
  const unsigned hp[2]={0x10b2c,0x12b2c};w[hp[0]]=w[hp[1]]=176;w[hp[victim]]=29;
  uint64_t damage[2]{},contacts[2]{};Tracker t;
  t.advance(vf2_haptic_snapshot(w,1,176,damage,contacts),3,75);
  w[hp[victim]]=0;damage[victim]+=29;++contacts[victim];
  auto ko=t.advance(vf2_haptic_snapshot(w,2,176,damage,contacts),3,75);assert(ko[0].knockout && ko[1].knockout);
  w[0x31]=17;w[0x35]=0;++contacts[victim];
  auto s=vf2_haptic_snapshot(w,3,176,damage,contacts);auto post=t.advance(s,3,75);
  assert(post[0].ms && post[1].ms && !post[0].knockout && !post[1].knockout);
  assert(!t.advance(s,3,75)[0].ms);
  s.frame=4;assert(!t.advance(s,3,75)[0].ms); // no contact, no vibration
  w[0x31]=13;w[0x35]=32;++contacts[victim];
  auto later=t.advance(vf2_haptic_snapshot(w,5,176,damage,contacts),1,75);assert(later[0].ms && !later[1].ms);
  w[0x2b]=3;++contacts[victim];assert(!t.advance(vf2_haptic_snapshot(w,6,176,damage,contacts),3,75)[0].ms);
  w[0x2b]=9;assert(!t.advance(vf2_haptic_snapshot(w,7,176,damage,contacts),3,75)[0].ms);
  ++contacts[victim];s=vf2_haptic_snapshot(w,8,176,damage,contacts);auto muted=t.advance(s,3,0);assert(muted[0].low==0 && muted[1].high==0);
 }
 // Missing post-KO damage uses the same pulse as an ordinary saturated hit
 // (>= maximum/8), preserving gain, player roles and ordinary duration, not a KO tail.
 for(int maximum : {176,200,240}) for(int gain : {0,25,75,100}) for(int victim=0;victim<2;++victim) {
  Tracker normal,post;Snapshot a{1,0,true,{maximum,maximum},maximum,3};a.damage_events=true;
  Snapshot b=a;b.health[victim]=0;normal.advance(a,3,gain);post.advance(b,3,gain);
  ++a.frame;++b.frame;int damage=(maximum+7)/8;a.health[victim]-=damage;
  a.damage_total[victim]=damage;++a.contact_total[victim];++b.contact_total[victim];
  auto n=normal.advance(a,3,gain),p=post.advance(b,3,gain);
  for(int player=0;player<2;++player) {
   assert(n[player].low==p[player].low && n[player].high==p[player].high);
   assert(n[player].ms==p[player].ms && !p[player].knockout);
   assert(p[player].low<=1 && p[player].high<=1);
  }
 }
 puts("PASS: VF2 zero-health contacts, observed post-KO phases, routing, no duplicate/setup/attract/stale pulses");
}
