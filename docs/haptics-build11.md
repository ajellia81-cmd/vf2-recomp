# Personal build 11: cold haptics preparation

Source: 57b25350e6fecfd0ef3b08bf3955076f017d9cc0
Successful workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39946785692
IPA: VirtuaFighter2-build11-unsigned.ipa (6,151,049 bytes)
SHA-256: 8049f879a6243e8a25136e52c0e5e7975fd4e80274dca4a15281417c8da02d1c

The phone engine/player prepare on a serial queue before impacts. The pinned SDL
Apple controller driver also prewarms native motors off the render thread and
outside the joystick lock, using the same implementation tested in personal Daytona
build 8. Stops, disconnects and resets supersede queued work; phone impacts that
are stale after cold initialization are dropped. Foreground engine warmth does
not enable vibration while the app is backgrounded or feedback is disabled.

Apple CI passed fresh device/simulator dependency compilation, Model 2 Metal and
display/controls checks, actual macOS execution of the controller concurrency
harness (blocked initialization, 1000 coalesced requests, stop, disconnect during
initialization/update, retry and recovery), scalar diagnostic ring bounds/export,
health tracker/adapter and touch Menu checks, iPhone Release compilation and both
iOS Simulator import XCTest cases. This is not a physical haptics/gameplay test.

Downloaded artifact and IPA digests match GitHub/runner output. ZIP integrity,
bundle/build/source metadata, interpreter-only policy and no game archives were
verified. The executable contains VFCombatIOSRumbleContext, VFFightPhoneHaptics and
the Save hit diagnostics menu, confirming that the patched SDL was linked.

## Unresolved request: post-KO contacts without health changes

Automatic detection still requires a verified contact event independent of health
loss. No speculative animation/button/sound trigger was enabled. Read-only scalar
health-store observations are retained in a fixed ring and exported with Controls >
Save hit diagnostics. The file is haptics-hit-diagnostics.csv in
Library/Application Support/Virtua Fighter 2 Recomp within the app data container.
Reproduce the missing post-KO impact, then export promptly. No game program bytes,
ROM data or full memory dump are captured. This evidence is needed before enabling
zero-damage contacts and excluding replay/initialization stores reliably.
