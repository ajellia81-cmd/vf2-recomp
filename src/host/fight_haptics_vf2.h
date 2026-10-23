#pragma once
#include "fight_haptics.h"
#include <vector>
// Read-only work RAM: tools/vf2run.cpp KO checks and scene_sync.h fight gate.
inline fight_haptics::Snapshot vf2_haptic_snapshot(const std::vector<uint8_t> &work, uint64_t frame, int maximum) {
 fight_haptics::Snapshot s; s.frame = frame;
 if (work.size() < 0x12b2e) return s;
 auto word = [&](unsigned a) { return work[a] | (work[a+1] << 8); };
 s.maximum = maximum;
 s.health[0] = int16_t(word(0x10b2c)); s.health[1] = int16_t(word(0x12b2c));
 s.live = work[0x2b] == 9 && work[0x31] == 9 && work[0x35] == 2;
 // Player routing is an explicit user choice; this adapter does not infer human sides.
 s.players = 3;
 return s;
}
