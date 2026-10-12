// Clean scene changes, as vf1-recomp's scene_sync does it for VF1, with VF2's own signals. It
// only decides which picture is shown: the game runs on untouched (vf2run and the MAME
// comparisons never use it).
//
// 1. The backdrop's two frames. VF2 scrolls its 2D backdrop (tile layer 2) for a camera two
//    frames after MAME draws that camera's 3D: at every camera cut (the attract's demos, round
//    intros, knockouts, replays) MAME shows the new shot over the old shot's sky for two frames,
//    and while the camera pans the sky trails the 3D by two frames' movement. So while a stage's
//    backdrop is up (layer 2's vertical scroll register set) and the players aren't fighting,
//    the 3D is shown two frames late (model2a::set_3d_delay), with the backdrop it was set for.
//    While they fight (the game in play, 0x50002b = 9, and the round on, 0x500035 = 2) the 3D
//    is shown as MAME shows it: no delay where the controls count, and there the camera follows
//    the fighters at a pixel a frame or so, with no cuts. With smooth motion the delay moves
//    gradually (the 3D a little slower or faster for a moment, the fraction carried by the
//    pictures between frames), and at a new shot's first frame it gives up what's needed for no
//    picture to be shown twice; without, a frame at a time (a frame held going up, one skipped
//    going down).
// 2. Scene starts. A stage's backdrop comes up a few frames before its 3D, and the first 3D
//    frame can still have the last scene's poses: when a stage shows no 3D, the last complete
//    picture stays up (or the game's own black curtain), until the polygon count has settled
//    (within a quarter of two frames before, or with smooth motion of the frame before when the
//    two pair as one shot), 30 frames at most.
// 3. A cut's first frame. Some new shots come with their fighters a frame late. With the delay
//    the next frame's 3D is already drawn: at a cut (the backdrop jumping), a 3D with under
//    three quarters of the next one's polygons is skipped (the last picture stays a frame).
// For this every frame's 3D is set up, but only the frames shown are drawn (model2a::defer_3d).
#pragma once
#include "../machine/model2a.h"
#include <cstdlib>

class scene_sync
{
public:
	bool enabled = true;

	// After a frame has run, before its picture is drawn: sets the 3D's delay for it and says
	// whether to show it (false: the last picture shown stays up, unless it's black: is_blank).
	bool show(model2a &m)
	{
		if (!enabled) {
			if (m_delay) m.set_3d_delay(m_delay = 0);
			m_delayf = 0;
			m.set_3d_shift(0);
			m_holding = false;
			return true;
		}
		const std::vector<u8> &w = m.workram();
		const u16 backdrop = m.tiles().tile_r(0x5006);
		const bool stage = backdrop != 0 && !(backdrop & 0x8000);
		const bool fighting = w[0x2b] == 0x09 && w[0x35] == 0x02;
		const int target = stage && !fighting ? 2 : 0;
		if (m.smooth()) {
			// With smooth motion the delay changes gradually: the 3D shown a little slower (going up,
			// 2 frames over 32: 94% speed) or faster (going down, over 20: 110%), smooth motion's
			// pictures between the frames carrying the fraction (set_3d_shift). No picture held or
			// skipped, and a KO's slow motion (where it mostly goes up) shows it least.
			m_delayf = std::min(m_delayf, m.delay_3d());   // (what smooth motion gave up at a new shot)
			const float step = target > m_delayf ? 1.0f / 16 : 1.0f / 10;
			m_delayf = target > m_delayf ? std::min(float(target), m_delayf + step) : std::max(float(target), m_delayf - step);
			m_delay = std::min(2, int(m_delayf + 1e-4f));
			m.set_3d_delay(m_delay, false);
			m.set_3d_shift(m_delayf - float(m_delay));
		} else {
			// Without (no lists kept), a frame at a time: going up the 3D holds for a frame, going
			// down it skips one, two single steps ten frames apart.
			if (target > m_delay) m_delay++;
			else if (target < m_delay && (m_since_step >= 10 || m_delay == 2)) m_delay--, m_since_step = 0;
			m_since_step++;
			m_delayf = float(m_delay);
			m.set_3d_delay(m_delay);
		}
		const int q = m.frame_polygons(), q1 = m_q1, q2 = m_q2;
		m_q2 = m_q1;
		m_q1 = q;
		const u16 h = m.tiles().tile_r(0x5002), v = backdrop;
		const int dh = std::abs(((h - m_h + 256) & 511) - 256), dv = std::abs(int(v & 0x1ff) - int(m_v & 0x1ff));
		const bool cut = stage && (m_h != h || m_v != v) && (dh > 24 || dv > 24);
		m_h = h;
		m_v = v;
		if (!m_holding) {
			if (stage && q == 0 && !fighting) { m_holding = true; m_held = 1; return false; }
			if (cut && m_delay == 2) {
				const int next = m.polygons_drawn(1);
				if (next > 0 && q * 4 < next * 3) return false;
			}
			return true;
		}
		// (with smooth motion's pairing, two frames of 3D in a row that pair as one shot will do: the
		// first no longer has the last scene's poses)
		const bool settled = q > 0 && ((q2 > 0 && std::abs(q - q2) * 4 <= q) ||
			(m.smooth() && q1 > 0 && std::abs(q - q1) * 4 <= q && !m.newest_3d_cut()));
		if (settled || !stage || ++m_held > 30) {
			m_holding = false;
			// a new scene shown after a hold: with smooth motion, it starts with the delay it wants
			// (the frames it takes from were run during the hold, unseen), so its first pictures
			// step evenly, with frames to blend toward, not a frame's own and then a small step
			if (m.smooth() && settled && stage && target > m_delayf) {
				m_delayf = float(target);
				m_delay = target;
				m.set_3d_delay(m_delay, false);
				m.set_3d_shift(0);
			}
			return true;
		}
		return false;
	}
	int delay() const { return m_delay; }
	float delay_exact() const { return m_delayf; }

	// A picture that is all but black (the game's curtain between scenes): shown even in a hold.
	static bool is_blank(const bitmap_rgb32 &b)
	{
		const uint32_t *p = b.data();
		const size_t n = size_t(b.width()) * b.height();
		for (size_t i = 0; i < n; i += 97) {
			const uint32_t c = p[i];
			if (((c >> 16) & 0xff) > 12 || ((c >> 8) & 0xff) > 12 || (c & 0xff) > 12) return false;
		}
		return true;
	}

private:
	int m_delay = 0;
	bool m_holding = false;
	float m_delayf = 0;      // the delay with its fraction (smooth motion)
	int m_since_step = 0;   // frames since the delay last went down
	int m_held = 0, m_q1 = 0, m_q2 = 0;
	u16 m_h = 0, m_v = 0;   // the backdrop's scroll last frame
};
