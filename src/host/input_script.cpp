#include "input_script.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

struct button { const char *name; int port; uint8_t mask; };
// MAME's vf2 input ports: IN0 coins/test/service/starts, IN1 and IN2 the players
const button BUTTONS[] = {
	{ "coin1", input_script::IN0, 0x01 }, { "coin2", input_script::IN0, 0x02 },
	{ "test", input_script::IN0, 0x04 }, { "service", input_script::IN0, 0x08 },
	{ "start1", input_script::IN0, 0x10 }, { "start2", input_script::IN0, 0x20 },
	{ "p1punch", input_script::IN1, 0x01 }, { "p1kick", input_script::IN1, 0x02 },
	{ "p1guard", input_script::IN1, 0x04 }, { "p1down", input_script::IN1, 0x10 },
	{ "p1up", input_script::IN1, 0x20 }, { "p1right", input_script::IN1, 0x40 },
	{ "p1left", input_script::IN1, 0x80 },
	{ "p2punch", input_script::IN2, 0x01 }, { "p2kick", input_script::IN2, 0x02 },
	{ "p2guard", input_script::IN2, 0x04 }, { "p2down", input_script::IN2, 0x10 },
	{ "p2up", input_script::IN2, 0x20 }, { "p2right", input_script::IN2, 0x40 },
	{ "p2left", input_script::IN2, 0x80 },
};

// "start1+p1punch" or "in1:11" -> (port, mask) pairs
bool parse_buttons(const std::string &s, std::vector<std::pair<int, uint8_t>> &out)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, '+')) {
		bool found = false;
		for (const button &b : BUTTONS)
			if (item == b.name) { out.push_back({ b.port, b.mask }); found = true; }
		if (found) continue;
		const size_t colon = item.find(':');
		if (colon == std::string::npos) return false;
		static const char *ports[] = { "in0", "in1", "in2", "sw" };
		int port = -1;
		for (int p = 0; p < input_script::N_PORTS; p++) if (item.substr(0, colon) == ports[p]) port = p;
		if (port < 0) return false;
		out.push_back({ port, uint8_t(strtoul(item.c_str() + colon + 1, nullptr, 16)) });
	}
	return !out.empty();
}

} // namespace

bool input_script::load(const std::string &path, std::string &error)
{
	std::ifstream in(path);
	if (!in) { error = "can't read " + path; return false; }
	uint64_t cursor = 0;
	std::string line;
	int lineno = 0;
	while (std::getline(in, line)) {
		lineno++;
		if (const size_t hash = line.find('#'); hash != std::string::npos) line.resize(hash);
		std::stringstream ls(line);
		std::string cmd, arg;
		if (!(ls >> cmd)) continue;
		auto fail = [&]() { error = path + ":" + std::to_string(lineno) + ": can't read \"" + line + "\""; return false; };
		std::vector<std::pair<int, uint8_t>> b;
		if (cmd == "wait") {
			uint64_t n;
			if (!(ls >> n)) return fail();
			cursor += n;
		} else if (cmd == "at") {
			uint64_t f;
			if (!(ls >> f)) return fail();
			cursor = f;
		} else if (cmd == "press" || cmd == "hold" || cmd == "release") {
			if (!(ls >> arg) || !parse_buttons(arg, b)) return fail();
			uint64_t n = 6;
			if (cmd == "press") ls >> n;
			for (auto [port, mask] : b) {
				if (cmd != "release") events.push_back({ cursor, true, port, mask });
				if (cmd == "release") events.push_back({ cursor, false, port, mask });
				if (cmd == "press") events.push_back({ cursor + n, false, port, mask });
			}
			if (cmd == "press") cursor += n;
		} else if (cmd == "shot") {
			if (!(ls >> arg)) return fail();
			shots.insert({ cursor, arg });
		} else
			return fail();
		end = std::max(end, cursor);
	}
	std::stable_sort(events.begin(), events.end(), [](const event &a, const event &b) { return a.frame < b.frame; });
	return true;
}

void input_script::apply(uint64_t frame, uint8_t ports[N_PORTS])
{
	while (m_next < events.size() && events[m_next].frame <= frame) {
		const event &e = events[m_next++];
		if (e.press) m_held[e.port] |= e.mask;
		else m_held[e.port] &= ~e.mask;
	}
	for (int p = 0; p < N_PORTS; p++) ports[p] = uint8_t(0xff & ~m_held[p]);
}

bool input_script::write_events(const std::string &path) const
{
	FILE *f = fopen(path.c_str(), "w");
	if (!f) return false;
	static const char *ports[] = { "in0", "in1", "in2", "sw" };
	for (const event &e : events) fprintf(f, "%llu %s %02x %d\n", (unsigned long long)e.frame, ports[e.port], e.mask, e.press ? 1 : 0);
	fclose(f);
	return true;
}
