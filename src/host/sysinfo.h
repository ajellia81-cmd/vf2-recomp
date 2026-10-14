// The computer, for log.txt (what a player's log needs to show why the game is slow there), and
// Windows' scheduling of the game's threads.
#pragma once
#include <string>

namespace sysinfo {

// Windows: the game opts out of power throttling (EcoQoS: Windows 11 may run a program it
// judges unimportant on the efficiency cores at low clocks, and let its timers run coarse)
void tune();
// One line: the CPU (its kinds of core), the system, the GPUs
std::string describe();
// The GPUs by name and memory (Windows: DXGI's adapters), as in describe()'s line
std::string gpu_names();
// Where the game's threads ran (Windows, on a CPU with performance and efficiency cores: Intel's
// 12th generation on): the share of the drawing's bands, and of the game's frames, run on the
// performance cores since the last call. Empty where that isn't known.
void note_frame();   // (the game's thread, at each frame)
std::string placement();

}
