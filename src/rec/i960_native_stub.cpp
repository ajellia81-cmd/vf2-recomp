// iOS-safe implementation of the native-recompiler boundary.  This file keeps the
// core linkable when SLJIT is intentionally omitted (Makefile: JIT=0).
#include "i960_native.h"

namespace i960rec {

// No native allocation can exist in this build. The runtime destructor still links here.
void free_native(void *) {}

bool build_native(runtime &, uint32_t (*)(void *, uint32_t), void *, uint32_t,
	const hints &, native_stats *stats)
{
	if (stats) *stats = {};
	return false;
}

} // namespace i960rec
