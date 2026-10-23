#pragma once
#include "fight_haptics.h"
#include <vector>
// Read-only work RAM: tools/vf2run.cpp KO checks and scene_sync.h fight gate.
inline fight_haptics::Snapshot vf2_haptic_snapshot(const std::vector<uint8_t> &work, uint64_t frame, int maximum, const uint64_t *damage_total=nullptr, const uint64_t *contact_total=nullptr) {
 fight_haptics::Snapshot s; s.frame = frame;
 if (work.size() < 0x12b2e) return s;
 auto word = [&](unsigned a) { return work[a] | (work[a+1] << 8); };
 s.maximum = maximum;
 if (damage_total && contact_total) {
  s.damage_events=true;
  for(int p=0;p<2;++p) { s.damage_total[p]=damage_total[p]; s.contact_total[p]=contact_total[p]; }
 }
 s.health[0] = int16_t(word(0x10b2c)); s.health[1] = int16_t(word(0x12b2c));
 s.live = work[0x2b] == 9 && ((work[0x31] == 9 && work[0x35] == 2) ||
  (s.damage_events && (s.health[0] == 0 || s.health[1] == 0)));
 // After KO, only verified contact-store counter increments can emit an impact.
 // Match-mode replay contacts use this same store; no unverified replay flag is guessed.
 // Player routing is an explicit user choice; this adapter does not infer human sides.
 s.players = 3;
 return s;
}
