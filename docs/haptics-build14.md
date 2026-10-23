# Personal iOS build 14: optional sound/contact diagnostics

Source: `b7f6d7c5055c36d927a1c838b76d6433c05a89a3` on the personal
`ios-port` branch. Runtime recording: `5852bf262275ceaa3c6a6e189838d086cd196c9e`.
Analysis and instructions: `6a722c8465995bb1249c9463b7a776c06effc83a`.

This build adds an opt-in 4096-row scalar recording of confirmed contact-store
observations and candidate channel-14 voice-start requests. It does not map
sound IDs to haptic strength: build 13's fixed post-KO fallback remains in use.
Music/weather are excluded, but swishes share the candidate channel. See
`ios/HAPTICS.md` for capture instructions and interpretation limits.

Linux verification: C++ capture tests passed in release and ASan/UBSan builds
(leak checking disabled because the runtime cannot inspect /proc tasks). The
four Python tests passed, including ambiguity and clipped-hit exclusions.
Existing combat probe, tracker, VF2 adapter and contact-store tests passed.
An interpreter-only headless tool compiled with VF2_IOS, correctly rejected a
missing archive and contained no SLJIT symbols. No game data was used.

Apple verification: workflow run
https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39989812501
and job 120538358995 succeeded. Metal/display/controls tests, nonblocking rumble
and capture tests, conservative sound analysis tests, combat haptic tests, touch
Menu checks, unsigned device Release compilation and both simulator document
import tests passed. These do not validate actual VF2 sound IDs, device audio
latency, physical haptic strength or gameplay.

Artifact: `VirtuaFighter2-build14-unsigned.ipa`, 6,153,650 bytes.
SHA-256: `561064b36e46c4763270a1a84c4da5cee056763a67210c5e5e3ba84671efd627`.
Actions artifact 12517402494 wrapper SHA-256:
`1f04fbfb161f020897befa22f6b825a1137035fa446c36e382b26637419e8d6d`.
Both ZIPs were checked; the inner SHA matched the runner log. Bundle version 14,
display name, source, interpreter-only/no-ROM metadata, disabled sound-strength
mapping and diagnostic menu/native haptic markers were inspected. No game
archives or ROM files were present. Unsigned personal use only; no public release.
