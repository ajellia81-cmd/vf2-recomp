#include "../../src/host/fight_haptics_vf2.h"
#include <cassert>
int main() {
 std::vector<uint8_t> w(0x30000);
 w[0x2b]=9; w[0x31]=9; w[0x35]=2;
 w[0x10b2c]=170; w[0x12b2c]=160;
 auto s=vf2_haptic_snapshot(w,12,200);
 assert(s.live && s.frame==12 && s.health[0]==170 && s.health[1]==160 && s.maximum==200);
 w[0x2b]=3; assert(!vf2_haptic_snapshot(w,13,200).live); // attract
 w[0x2b]=9; w[0x35]=0; assert(!vf2_haptic_snapshot(w,14,200).live); // replay/intro
 w[0x35]=2; w[0x31]=0; assert(!vf2_haptic_snapshot(w,15,200).live);
 assert(!vf2_haptic_snapshot({},16,200).live);
 fight_haptics::Tracker tracker; w[0x31]=9;
 tracker.advance(vf2_haptic_snapshot(w,17,200),1,75);
 w[0x12b2c]-=8; assert(tracker.advance(vf2_haptic_snapshot(w,18,200),1,75)[0].ms);
}
