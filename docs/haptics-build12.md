# Personal build 12: post-KO contact feedback

Source: de0f1a906a9b93be07755cb93f08601e528a8bdc
Successful workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39956051945
IPA: VirtuaFighter2-build12-unsigned.ipa (6,151,291 bytes)
SHA-256: e5014360bd44f334e5cff7dc7ee6bd7e76977205ec5908940b855a74c337bece

The user-supplied scalar trace identifies guest PC 0x18A7C as the normal damage
writer and shows zero-to-zero writes there after KO. Complete health stores from
that PC in match mode increment per-victim contact counters. Available damage
is retained separately; zero-damage contacts use a fixed light response. The
frame tracker consumes counters once per original frame, with no health/event
double counting and no repeated KO tail. No audio trigger or game-memory writes
are introduced. Phone/controller cold-start preparation from build 11 is retained.

Apple CI passed native Metal/display/controls checks, the nonblocking controller
concurrency harness, scalar diagnostic bounds/export, haptic tracker/adapter and
new contact tests, touch Menu tests, iPhone Release compilation, both iOS
Simulator import XCTest cases, interpreter-only packaging and artifact upload.
Linux synthetic tests also passed. Tests use no game data.

The downloaded artifact digest matches GitHub, and the IPA digest matches the
runner. Both ZIP containers passed integrity checks. Bundle version, source,
interpreter-only/no-ROM metadata and absence of game archives were verified.
The executable contains both native haptic backends and the diagnostic menu.

Physical iPhone/controller feedback and game-transition timing remain untested
here. The trace does not independently identify replay scenes: match-mode replay
contacts using this same writer may also vibrate. The documentation explicitly
preserves this limitation rather than guessing a replay flag.
