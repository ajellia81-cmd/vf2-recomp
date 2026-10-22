# Personal unsigned IPA build receipt

- Authorized by the user on 2026-10-22.
- Repository/branch: `ajellia81-cmd/vf2-recomp`, `ios-port`; upstream was not modified.
- Source: `faedf53e2c8dd4f8ca8fd8da5ea3bf475ffa5774`.
- Successful packaging run: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39737268286
- Output: `VF2Recomp-2868x1320-unsigned.ipa` (4,533,233 bytes).
- SHA-256: `156f4d941c7b4e769bd34834e0d10c862149fcf829193ad394d2750b373153a8`.
- Device target: arm64 iPhoneOS, Release, minimum iOS 15.
- Bundle identifier: `com.ajellia81.vf2recomp`.

The workflow compiled an unsigned device app and packaged it under `Payload/`.
It did not use Apple signing credentials, provisioning profiles, an Xcode
archive/export, or JIT entitlements. The IPA needs personal re-signing before
installation. No ROM or extracted game data was obtained or included. No release
was published. Dependency notices and source metadata are included in the app.

Actual checks passed: native 2868 x 1320 synthetic Metal rasterization in Exact
and Fast modes, project/source audit, device compilation, arm64/platform checks,
absence of SLJIT symbols, and IPA ZIP integrity/metadata checks. The downloaded
IPA's SHA-256 matched the runner output. This does not establish successful ROM
import, gameplay, controls, audio, frame rate or thermal behavior on an iPhone.

`IOS_VALIDATION.md` records the earlier compile-only preparation stage. This
receipt records the subsequently authorized personal packaging stage. Render
resolution remains fixed at 2868 x 1320 for battles; execution remains
interpreter-only.

## ROM import correction (build 2)

- Source: `5de5459324623889ab2a3bb8b4faf8cd0452c1b4`.
- Packaging and simulator tests: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39744951789
- Device/simulator compile checks: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39744951628
- Output: `VF2Recomp-2868x1320-import-fix-unsigned.ipa` (4,535,805 bytes).
- SHA-256: `332152e53559e7ef9e8f19d068f50d51e05cafcf44b645602b01b5a1b068a78e`.

The picker imports a provider-managed copy and permits generic data documents,
then checks the actual ZIP/ROM contents. Coordinated copying and validation run
in a background queue, with progress shown while UIKit stays on the main loop.
Completion and modal transitions run on the main thread. The bundle identifier
is unchanged and the build number is 2.

Two actual iOS Simulator XCTest cases passed: invalid plain-text document import
returns before its error callback, completes on the main thread and preserves
the source; cancellation completes once. The tests do not contain ROM data.
Native 2868 x 1320 synthetic Metal tests, unsigned Release device compilation,
unsigned Debug simulator compilation, packaging and downloaded ZIP/hash checks
also passed. Physical-device document-provider interaction, tapping Open, valid
ROM import and gameplay remain unverified; no claim is made that the user's
specific picker failure was reproduced on their device.

## App identity and conditional touch controls (build 3)

- Source: `9dbfd76d245dda3c836084b41806fe72c047518c`.
- Successful IPA workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39747373963
- Successful device/simulator compile checks: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39747374241
- Output: `VirtuaFighter2-build3-unsigned.ipa` (6,132,437 bytes).
- SHA-256: `0305240296bb756c0082d28bb67ba7b6383edc8083eb013328d0d70535e82dba`.

Display and bundle names are `Virtua Fighter 2`. The user's original opaque,
white-background artwork is converted into an AppIcon asset catalog. The
completed IPA contains compiled icon assets and declares AppIcon as its primary
icon; its downloaded ZIP, source metadata, names, version and hash were checked.

The touch overlay is attached after SDL creates its render view and kept in
front when visible. It shows directions, Guard/Punch/Kick, Coin/Start/Menu only
without a connected controller; connecting/disconnecting clears held touch
input and changes visibility. Menu mode retains only the Menu button while
no controller is connected. The bundle identifier is unchanged for updates.

Native 2868 x 1320 Metal tests, two document-import XCTest cases and both Apple
SDK compilation configurations passed. Actual touch gestures and controller
hot-plug behavior for this build remain unverified on a physical iPhone. The
user reported successful execution of the preceding import-fix build. Execution
remains interpreter-only, and no ROM is included.

## Display controls and redraw correction (build 4)

- Source: `7c833bdd9cb4f0d176faeda35aff402368cbf45a`.
- Successful IPA workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39766806993
- Successful device/simulator compile checks: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39766806999
- Output: `VirtuaFighter2-build4-unsigned.ipa` (6,134,950 bytes).
- SHA-256: `d222a5667324a21ef00e4345fa85201e0615d4f80f3159bff4cfbeb702d78f93`.

Display > Resolution now offers 1x through 6x and direct 2868 x 1320, saved in
`ios_resolution`. Scale changes finish outstanding work and recreate targets.
Display > Brightness ranges from 50% to 200% in 10% steps, saved separately;
100% preserves the game picture's RGB and higher values increase it before SDR
clipping. The menu and UIKit touch controls are not modulated.

The background-filter menu's CPU redraw was incorrectly treated as a fatal
native-resolution violation. That check is removed. Setting redraws complete
through the GPU immediately, including when normal frames are pipelined, and
are not overwritten by the CPU reference texture. Every composition clears
its target before drawing to prevent uncovered/uninitialized pixel retention.

A real macOS SDL/Metal synthetic test passed exact RGB readback for background
and premultiplied 3D layers, filter Off/Smooth/Off, 1x-6x/native switches, two
compositor lifetimes, and 100/150/200/100% brightness. This uses host SDL from
Homebrew; the app itself retains the pinned static iOS SDL dependency. Existing
native Metal raster tests, two iOS Simulator import tests, ROM-free interpreter
checks, Release device/Debug simulator compilation and IPA/hash checks passed.

No iOS-specific brightness attenuation was found in source inspection; the
board gamma mapping remains the original renderer's mapping. Synthetic color
checks do not establish actual game's brightness or color accuracy. The user's
separate report of graphics corruption on every real-device relaunch is not
reproduced or confirmed fixed by these tests. A screenshot of that condition
was requested. No ROM or NVRAM data was obtained, modified or deleted.
