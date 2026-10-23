#pragma once
#include <cstdint>
// Observed damage store PC in the user's build 11 scalar trace. Setup uses other PCs.
inline bool vf2_contact_store(uint32_t pc, bool full_health_word, int before, int after, unsigned mode) {
 if (mode != 9 || pc != 0x00018a7c || !full_health_word) return false;
 return (before > after && after >= 0) || (before == 0 && after == 0);
}
