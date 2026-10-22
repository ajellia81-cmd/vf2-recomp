# iOS preparation validation

## Verified source revision

- Repository: `ajellia81-cmd/vf2-recomp`
- Branch: `ios-port`
- Compiled commit: `06530e6abf4629758b7d46f3a73ae8435d37853a`
- Successful workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39602720858
- Workflow completion recorded at 2026-10-21 13:08 UTC (22:08 KST).
- This document records results; it does not change the compiled source/configuration.

## Passed checks

| Check | Actual evidence |
| --- | --- |
| Linux interpreter core | ROM-free `vf2run` compiled with `JIT=0`; missing-archive diagnostic returned the expected failure; no SLJIT symbols found |
| Dependencies | Pinned static SDL3 and FreeType XCFrameworks for arm64 device and simulator passed slice/header preflight |
| Project generation | XcodeGen project generated; source/resource/interpreter configuration audit passed |
| iPhone compilation | Unsigned Release application compiled for generic arm64 iPhoneOS with `CODE_SIGNING_ALLOWED=NO` |
| Simulator compilation | Unsigned Debug application compiled for generic arm64 iPhoneSimulator with `CODE_SIGNING_ALLOWED=NO` |

Both workflow jobs and both Apple compilation steps concluded successfully.
Dependency libraries were restored from the prepared cache. No ROM was used in
these checks. No app archive, IPA export, app artifact upload, or release was
created.

## Prepared behavior, not yet runtime-verified

Source integration includes sandbox ZIP import and validation, separate
Application Support persistence, touch and GameController input, menu input
handling, lifecycle save/audio handling, and the shared SDL Metal layer.
CPU 3D remains the initial default. iOS remains interpreter-only in Debug and
Release; SLJIT and the native compiler are excluded.

The app was **not launched** in this workflow. ROM import, actual gameplay,
controller/touch behavior, save recovery, audio interruptions, Metal output,
frame rate and thermal behavior have not been tested on a simulator or device.
No physical iPhone build/run or personal signing has been tested.

## Stop point

Repository preparation and unsigned Apple SDK compilation are complete.
Personal signing/provisioning and any archive/IPA packaging remain a later,
explicitly requested step. Preserve the no-ROM and interpreter-only policies.
See `ios/README.md` for reproduction and device-validation steps, and
`AGENTS.md` for scope.
