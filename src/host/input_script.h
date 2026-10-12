// A script of inputs, frame by frame, for driving the game headlessly (vf2run --script).
//
//   # comment
//   wait N                    let N frames go by
//   press BUTTONS [N]         hold them for N frames (default 6), then let go; the N frames go by
//   hold BUTTONS              keep them held from now on ...
//   release BUTTONS           ... until this
//   shot NAME                 write the frame just drawn as <out>/NAME.png
//   at FRAME                  go to that frame (the frames in between go by)
//
// BUTTONS: names joined with '+': coin1 coin2 test service start1 start2 (IN0),
// p1punch p1kick p1guard p1down p1up p1right p1left (IN1), the same with p2 (IN2), or
// raw PORT:MASK with PORT in0/in1/in2/sw and MASK in hex (in1:01). Inputs are active low on the
// board: a held button clears its bit. Frame numbers are MAME's: the inputs set "at frame F"
// are what the game reads while frame F + 1 runs, as when MAME's Lua sets them at frame F's end.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct input_script
{
	enum port { IN0, IN1, IN2, SW, N_PORTS };
	struct event { uint64_t frame; bool press; int port; uint8_t mask; };   // press: clear the bits
	std::vector<event> events;                    // in frame order
	std::multimap<uint64_t, std::string> shots;   // after frame F is drawn
	uint64_t end = 0;                             // the frame the script ends at

	// Parses the script; on error, returns false with the line in `error`.
	bool load(const std::string &path, std::string &error);
	// The port values for the frame after `frame` (the ones the game reads while it runs).
	void apply(uint64_t frame, uint8_t ports[N_PORTS]);
	// The events as "frame port mask press" lines, for tools/mame_snap.sh (SCRIPT_EVENTS=file).
	bool write_events(const std::string &path) const;

private:
	size_t m_next = 0;
	uint8_t m_held[N_PORTS] = {};
};
