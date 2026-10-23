# Personal build 13: post-KO contact intensity

Source: dd7abec06d8d29e84a7586eebf0a8a682e0e2882
Successful workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39986447590
IPA: VirtuaFighter2-build13-unsigned.ipa (6,151,270 bytes)
SHA-256: 6f4e96732bd1ffeda140d76eafc6a0a281c3148c4f675fd536477bf99ce6e7b2

The user's build 12 test reported weak feedback after KO. Confirmed contacts whose
damage was removed by zero-health clamping previously used weight 0.4, weakening
both amplitude and duration. They now use the ordinary saturated-hit profile,
weight 1.0. This is a deliberate fixed fallback, not a measured damage value.
The decisive KO alone retains its extra tail. Real damage, routing, strength gain,
duplicate suppression and off-thread native engine preparation remain intact.

At 75% strength, landed low/high/duration changes from 0.21/0.405/41 ms to
0.39/0.675/65 ms; received from 0.414/0.2625/64 ms to 0.675/0.4875/100 ms.
Phone intensity uses max(low,high). These are signal parameters, not measured
physical output or a linear prediction of perceived strength.

VF1 was audited separately: its verified pre-clamp damage preserves exactly the
ordinary-hit amplitude and duration for equal damage after KO, so build 6 needs
no runtime change. Both forks gained parity tests for player sides and strengths
0/25/75/100. VF1 covers damage 1/4/12/30; VF2 covers maxima 176/200/240. The native
phone, SDL motor preparation and routing sources were compared and are identical.

Apple CI passed the tracker/adapter/contact tests, nonblocking controller harness,
scalar diagnostic bounds, Metal/display/controls and touch Menu checks, iPhone
Release compilation and both iOS Simulator import XCTest cases. Linux checks also
passed. All automated checks use no game data. No physical-device feedback test
of this updated build was performed.

Downloaded artifact and IPA digests match GitHub/runner output. Both ZIPs passed
integrity checks; bundle/build/source metadata, interpreter-only policy, absence
of game archives and presence of both native feedback backends were verified.
The existing possibility of feedback for match-mode replay contacts is unchanged.
