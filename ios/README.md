# Personal iOS port

Work only in `ajellia81-cmd/vf2-recomp`, branch `ios-port`. Upstream
`mr-finster/vf2-recomp` is read-only. `project.yml` is the source of truth;
generated Xcode projects, dependencies and build products stay ignored.

No ROM, extracted game data or generated game source is included or fetched.
The compile-check workflow never packages an IPA. Personal IPA packaging is a
separate, explicitly authorized workflow described below.

## Reproduce on a macOS cloud runner

Requirements: Xcode with iPhoneOS/iPhoneSimulator SDKs, XcodeGen 2.38+,
CMake 3.24+, Git and Python 3. No Windows machine or signing secrets are needed
for dependency preparation or unsigned compilation.

```sh
sh ios/Scripts/prepare-dependencies.sh
sh ios/Scripts/preflight.sh
python3 ios/Scripts/prepare-app-icon.py
xcodegen generate --spec ios/project.yml
python3 ios/Scripts/verify-project.py
```

The GitHub workflow `.github/workflows/ios-compile.yml` runs these commands,
checks the ROM-free interpreter tool on Linux, and compiles unsigned Release
for arm64 iPhoneOS and Debug for arm64 iPhoneSimulator. It does **not** run
the app, create an archive, export an IPA, upload app artifacts, or publish a
release. Redistributable static dependency libraries are cached separately.
Check the workflow result for the exact commit; a source commit is not proof
of a successful Apple SDK build.

## Personal unsigned IPA

The user authorized personal IPA creation on 2026-10-22. The separate workflow
`.github/workflows/ios-personal-ipa.yml` builds Release for arm64 iPhoneOS,
checks interpreter-only symbols and device platform, adds dependency notices,
and packages the app under `Payload/` without ROMs or signing credentials.
It runs manually or when its own workflow file changes on `ios-port`, only in
the personal fork. The artifact expires after three days; no release is published.

The resulting `VirtuaFighter2-build5-unsigned.ipa` requires personal re-signing
before installation. It contains no provisioning profile and no JIT entitlement.
The source commit is recorded inside `BUILD-INFO.json`. Packaging does not prove
working gameplay or device performance. The original compile-only workflow
remains separate and does not upload applications.

## Execution and rendering

`VF2_DISABLE_JIT=1` is mandatory for both configurations. The target excludes
`i960_native.cpp` and SLJIT and includes `i960_native_stub.cpp`, including its
runtime teardown boundary. An iOS compile of the native compiler deliberately
fails. The MAME i960 interpreter is selected without executable-memory
allocations. Do not enable JIT entitlements or ROM-generated recompilation.

The Metal backend is linked using `VF2_GPU3D_METAL=1` and shares SDL's
`CAMetalLayer`. Display > Resolution offers the original 1x through 6x render
scales and direct **2868 x 1320** rendering. The native option uses fractional
per-axis vertex/clipping coordinates and a matching composition target rather
than enlarging lower-resolution 3D. Original 2D artwork is sampled into that
composition. The selected resolution is saved separately as `ios_resolution`;
old installs retain the native option as their initial selection. Resolution
changes finish pending work and recreate both GPU and composition buffers.

Background-filter changes redraw through the GPU while the menu is paused.
A temporary CPU redraw or non-battle transition is not treated as a native-size
failure. Every composition clears its target first so uncovered backdrop pixels
cannot retain prior/uninitialized data. CPU execution remains interpreter-only;
changing the render scale never enables JIT.

Display > Brightness adjusts the completed game picture from 50% to 200%, with
100% preserving original RGB values. Values above 100% brighten RGB before SDR
clipping, keeping black at black. Menus and touch controls are not modulated.
Screen filter is separate: Scanlines and CRT intentionally alter brightness.
A ROM-free Metal/SDL test checks known RGB values, background-filter toggles,
integer/native resolution switches, compositor recreation, and brightness gain.
These checks do not establish correctness of actual gameplay or NVRAM-dependent
cold launches on a physical iPhone. No gameplay data is used by CI.

## ROM import and storage

On startup the host uses only
`Library/Application Support/Virtua Fighter 2 Recomp/roms/vf2.zip`.
If that archive is absent or invalid, a temporary SDL UIKit window presents
`UIDocumentPickerViewController` for ZIP files. The picker requests an imported copy and accepts generic data types so providers
that mislabel ZIP archives do not disable selection. The archive is still fully
validated by content. Copying and CRC checks run on a background queue with a
visible progress indicator; UIKit completion and dismissal stay on the main
thread. The source URL is accessed only during coordinated copying to a unique temporary sandbox file. The existing
`romset::load` validates names and CRCs; a successful copy atomically replaces
the sandbox archive. Failed copies/validation are removed and an existing
archive is preserved on installation failure. No external bookmark is stored.

Saves, configuration and logs use the same Application Support app directory,
separate from `roms/`. Desktop portable mode and external ROM-folder settings
do not redirect iOS persistence. User-imported archives never enter CI.

## Input and lifecycle

SDL owns the window, event pump, renderer, audio and menu navigation.
`VF2InputBridge` polls GameController on the main thread, maintains two stable
controller slots and merges physical inputs with a UIKit touch overlay. The
host samples it every outer loop, including while the game menu is paused.
SDL handles menu navigation; it does not duplicate GCController game inputs.

Default controller mapping: D-pad/left stick move; X/A/B = Guard/Punch/Kick;
Y = Guard+Punch; LB/LT = Guard+Kick; RB = Punch+Kick; RT = all three;
Menu = Start; Options = game menu. The touch overlay supplies directions,
Guard/Punch/Kick, Start, Coin and Menu. Controls respect window safe-area
insets; unused overlay space passes touches to SDL, and only Menu remains
visible while the game menu is open. The overlay is installed above the SDL
render view and shown only while no controller is connected. Connecting a
controller hides all touch controls and clears held touch input; disconnecting
restores the overlay. Accessibility scaling and comprehensive
controller remapping still need device review.

Lifecycle event watches pause audio/emulation, clear held touch inputs and
audio queues, flush pending writes and save current NVRAM/settings before
UIKit suspension. Foreground recovery resets the frame clock and resumes
audio according to menu state. Actual interruption and route-change behavior
must still be tested on an iPhone.

## Staged completion plan

| Gate | Repository preparation | Required evidence |
| --- | --- | --- |
| 1. Interpreter core | Native compiler excluded; ROM-free Linux tool/checks | Successful Linux build; missing-ROM diagnostic; no SLJIT symbols |
| 2. Apple compilation | Pinned static dependencies, source/resource audit, unsigned device/simulator workflow | Green jobs on the current branch head |
| 3. Device behavior | Import, persistence, touch/GCController, lifecycle and Metal boundary connected | User-owned archive tested on an actual device; no ROM sent to CI |
| 4. Personal packaging | Bundle ID `com.ajellia81.vf2recomp`; signing unset | Select personal signing method; include notices; explicit authorization before IPA generation |

A compilation pass establishes only gate 2. Do not claim working gameplay,
device performance or a verified IPA-ready build before the corresponding
evidence exists. Device validation includes bad-CRC import preserving prior
data, source-provider access revocation, two-controller disconnect/reconnect,
multitouch, menu gestures, rotation, background/relaunch and audio recovery.

## Cloud continuity

Commits and GitHub Actions checks persist independently of the user's PC.
This chat's remote workspace is not itself a published Codex Cloud environment
or a registered Codex Cloud task. For a separate Codex Cloud task, select the
personal repository and `ios-port` from a published environment, read
`AGENTS.md`, and continue from the branch head without touching upstream.
