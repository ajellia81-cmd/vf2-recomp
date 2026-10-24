#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

// A verified health store is mandatory. Sounds alone never emit feedback.
// The positive 8 ms window covers the measured request latency (1.18--5.47 ms).
// Multiple voices/contacts, weather and unknown IDs retain a measured/fixed fallback.
// Both lethal (clipped) and post-KO contacts can wait for an effect request.
class vf2_impact_sound {
 struct Contact {
  uint64_t us=0; int player=0, voices=0, note=-1;
  bool used=false, post=false, ambiguous=false;
  bool knockout=false; int damage=0;
 };
 std::array<Contact,16> contacts{};
public:
 struct Batch { uint64_t count[2]{}, knockouts[2]{}, damage[2]{}; int estimate[2]{}; bool unknown[2]{}; };
private:
 Batch ready{};
 void finish(const Contact &c, bool overflow=false) {
  if(!c.post)return;
  // User choice: all post-KO contacts use the strongest ordinary profile.
  // Only the decisive lethal hit may still use the build15 sound estimate.
  const int estimate=(c.knockout && !overflow && !c.ambiguous && c.voices==1) ? estimate_damage(c.note) : 0;
  ++ready.count[c.player];
  ready.knockouts[c.player] += c.knockout;
  ready.damage[c.player] += unsigned(std::clamp(c.damage,0,1000));
  ready.unknown[c.player] |= estimate==0;
  ready.estimate[c.player]=std::max(ready.estimate[c.player],estimate);
 }
public:
 static constexpr uint64_t window_us=8000;
 static int estimate_damage(int note) {
  // Provisional medians from >=4 nonlethal scalar observations per ID.
  // These are estimates, never added to measured damage counters.
  switch(note) { case 0x01: case 0x03:return 15;
   case 0x25:return 22; case 0x24:return 45; default:return 0; }
 }
 void clear() { contacts={};ready={}; }
 void advance(uint64_t us) {
  for(auto &c:contacts) if(c.used && us>=c.us && us-c.us>window_us) {
   finish(c);c.used=false;
  }
 }
 void contact(uint64_t us,int player,bool post,bool knockout=false,int damage=0) {
  if(player<0 || player>1)return;
  advance(us);
  auto slot=std::find_if(contacts.begin(),contacts.end(),[](const auto &c){return !c.used;});
  if(slot==contacts.end()) {
   slot=std::min_element(contacts.begin(),contacts.end(),[](const auto &a,const auto &b){return a.us<b.us;});
   finish(*slot,true); // bounded overflow preserves contacts at fallback strength
  }
  *slot={us,player,0,-1,true,post,false,knockout,damage};
 }
 void voice(uint64_t us,unsigned note) {
  if(note>255 || note==0x22 || note==0x28 || note==0x29)return;
  int matches=0;
  for(const auto &c:contacts) if(c.used && us>=c.us && us-c.us<=window_us)++matches;
  for(auto &c:contacts) if(c.used && us>=c.us && us-c.us<=window_us) {
   ++c.voices;c.note=int(note);c.ambiguous |= matches!=1;
  }
 }
 Batch take(uint64_t us) {
  advance(us);auto out=ready;ready={};
  for(int p=0;p<2;++p)if(out.unknown[p])out.estimate[p]=0;
  return out;
 }
};
