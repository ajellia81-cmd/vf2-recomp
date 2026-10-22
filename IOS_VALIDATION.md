# iOS preparation validation

## Verified source revision

- Repository: `ajellia81-cmd/vf2-recomp`
- Branch: `ios-port`
- Compiled commit: `4551027b496f83323e03a01d96a38634e4e60c97`
- Successful workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39734443642
- Validation date: 2026-10-22 UTC.
- This document records results; it does not change compiled source/configuration.

## Passed checks

| Check | Actual evidence |
| --- | --- |
| Pixel grid | ROM-free C++ checks cover legacy integer mapping, exact native target dimensions, fractional axis scales, viewport bounds and invalid inputs |
| Metal rasterization | Synthetic geometry rendered into a 2868 x 1320 target on a Metal device; Exact and Fast modes passed color and pixel-boundary readback checks |
| Linux interpreter core | ROM-free `vf2run` compiled with `JIT=0`; missing-archive diagnostic returned the expected failure; no SLJIT symbols found |
| Dependencies | Pinned static SDL3 and FreeType XCFrameworks for arm64 device and simulator passed slice/header preflight |
| Project generation | XcodeGen project generated; source/resource/interpreter configuration audit passed |
| iPhone compilation | Unsigned Release application compiled for generic arm64 iPhoneOS with `CODE_SIGNING_ALLOWED=NO` |
| Simulator compilation | Unsigned Debug application compiled for generic arm64 iPhoneSimulator with `CODE_SIGNING_ALLOWED=NO` |

The synthetic Metal test ran on the macOS Actions host, using generated geometric
primitives only. This is a graphics-backend runtime test, not a game or iPhone
runtime test. Both workflow jobs and both Apple compilation steps succeeded.
No ROM was used. No app archive, IPA export, app artifact upload or release was
created.

## Prepared behavior, not yet game-runtime-verified

On iOS, battle 3D rasterization and final composition use a fixed 2868 x 1320
pixel target. Metal vertices and viewport bounds use fractional axis scales
before rasterization. Original 2D artwork is composed at that output size;
its original texture detail is unchanged. Legacy scale, supersampling, aspect
and renderer settings cannot lower the requested battle resolution. Metal
failure reports an error instead of silently substituting a lower-resolution
CPU battle frame. Scene-transition holds remain in force.

Source integration also includes sandbox ZIP import and validation, separate
Application Support persistence, touch and GameController input, menu input,
lifecycle save/audio handling, and the shared SDL Metal layer. iOS remains
interpreter-only in Debug and Release; SLJIT and the native compiler are excluded.

The app was **not launched** in this workflow. ROM import, actual gameplay,
controller/touch behavior, save recovery, audio interruptions, the complete
SDL-to-screen composition path, frame rate and thermal behavior have not been
tested on a simulator or physical device. No personal signing has been tested.
The fixed render target alone does not establish gameplay performance or the
physical display's pixel dimensions.

## Stop point

Repository preparation and unsigned Apple SDK compilation are complete.
Personal signing/provisioning and archive/IPA packaging remain later,
explicitly requested steps. Preserve the no-ROM and interpreter-only policies.
See `ios/README.md` for reproduction and device-validation steps, and
`AGENTS.md` for scope.

The earlier preparation baseline also compiled successfully at
`06530e6abf4629758b7d46f3a73ae8435d37853a`:
https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39602720858.
Its CPU-default rendering policy is superseded by the fixed Metal battle policy
above.
