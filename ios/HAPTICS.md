# Personal iOS combat haptics

Controls > Vibration enables damage-responsive feedback (default on, strength 75%, Player 1).
Vibration strength, Vibration players and Test vibration are saved in settings.ini.
A controller with working SDL rumble receives its assigned player's feedback.
Otherwise the phone uses Core Haptics. With Both selected the shared phone follows P1;
select P2 to follow P2. Hardware without haptics remains silent.

Health loss is observed after original emulation frames, never interpolated display frames.
Intensity varies continuously with damage, with a sharper landed-hit response, a heavier
received-hit response and a longer zero-health finish. Concurrent impacts merge by maximum,
not addition; phone impacts replace old tails. Menu, pause, fast forward, background,
reset and controller changes cancel feedback and reset the baseline. The first frame,
health refill, skipped frames and invalid values cannot produce an impact.

This is an original implementation inspired by event-based fighting-game feedback,
not a reproduction of Tekken's unpublished algorithms. Guard, counter, throw, ring-out
and cinematic events are not inferred. Damage cheats may suppress or change feedback.

## Evidence and limits

Read-only energy addresses and round gate come from tools/vf2run.cpp's existing KO
check. scene_sync.h supplies the original active-fight gate. The post-KO
contact path described below relaxes that gate in match mode and cannot guarantee
replay exclusion.
The configured 1P/VS energy ceiling comes from model2a's existing backup settings.
Player sides are chosen explicitly in Controls; human/CPU ownership is not inferred.

Synthetic tests check event extraction, routing, strength, KO, duplicate frames, resets,
invalid values and adapter gates without game data. Apple CI checks compilation; physical
iPhone/controller feel and exact game-transition timing still require device testing.

## Cold-start preparation

Phone engine startup and player creation run on a dedicated serial queue, prepared
when feedback is enabled/foregrounded. The engine stays warm during foreground
play; background and exit dispose it. Stops replace queued hits; effects older
than 100 ms are dropped after cold initialization. Reset recovery is silent.
The pinned SDL Apple driver uses the same off-thread motor preparation and
coalesced state updates validated by the personal Daytona build 8. Opening a
controller no longer creates native engines under SDL's joystick lock.

This removes synchronous cold engine work from the rendering thread. Physical
frame pacing and haptic timing must still be checked on the user's device.

## Verified contact stores after KO (build 12)

The user's build 11 scalar trace shows normal damage at guest PC 0x18A7C and
zero-to-zero health stores from that same PC after KO (frame 2316: mode 9,
round 17, live 0; frame 3089: mode 9, round 13, live 32). Setup/refill uses other
PCs. Thus both frame-end health deltas and the old active-round gate miss these
contacts. The iOS bus now counts complete health-field stores from 0x18A7C in
human match mode 9. It records a positive damage total where available, and a
contact total even for zero-to-zero writes. It never modifies game memory.

The frame tracker consumes monotonic totals once per original frame, avoiding
health/event double counting. Zero-damage contacts use a fixed ordinary saturated-hit profile (build 13),
not a guessed damage amount. Only positive-to-zero health transitions get the
long KO tail. Player routing, gain, pause, menu and background cancellation stay.
No stores from setup PCs, partial health stores or attract mode can emit feedback.

After KO, contact-driven feedback continues while a fighter has zero health in
match mode. The supplied scalar trace does not independently label replay scenes:
match-mode replay contacts using the same verified store may also vibrate. No
unverified scene flag is presented as a reliable replay exclusion. Broader move,
scene and physical-device coverage still needs testing. VF1 uses its own independently
observed pre-clamp store, not these addresses. No ROM bytes or disassembly are used.


## Build 13 post-KO intensity correction

Build 12 used weight 0.4 for confirmed zero-damage contacts, versus the normal
curve's maximum weight 1.0. This unintentionally weakened and shortened feedback
when the game's clamp removed the damage value. Build 13 uses weight 1.0 for
these contacts: the same profile as an ordinary hit dealing at least maximum/8.
This is a chosen fallback, not a recovered damage measurement or a repeated KO.
Measured damage still follows the continuous damage curve. No KO bonus is added
when the victim was already at zero health. Player gain and routing remain active.

At strength 75%, landed feedback changes from low/high 0.21/0.405 for 41 ms to
0.39/0.675 for 65 ms; received feedback from 0.414/0.2625 for 64 ms to
0.675/0.4875 for 100 ms. Phone intensity is max(low, high), so both roles change
from 0.405/0.414 respectively to 0.675. Duration and amplitude are both corrected.
Synthetic tests compare fallback contacts with ordinary saturated hits for both
players, maxima 176/200/240 and strengths 0/25/75/100, including muted output and
amplitude bounds. No physical-device feedback test is implied.
