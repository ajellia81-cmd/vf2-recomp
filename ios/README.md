# iOS port scaffold

This directory is an intentionally ungenerated Xcode project draft.  `project.yml` is the source of truth for XcodeGen; do not commit a generated `.xcodeproj` until the toolchain, dependency packaging, and signing decisions below have been reviewed.

No ROM, extracted game data, generated recompilation output, or downloadable game asset is included here.  A user imports their own legal MAME `vf2.zip` through the system document picker. `VF2ROMImport` obtains security-scoped access only for the import, validates the archive with the existing `romset::load`, then atomically copies it into this app's Application Support sandbox. The app does not retain external bookmarks or access unrelated files.

## Layout

| Path | Purpose |
| --- | --- |
| `project.yml`, `Config/*.xcconfig` | Reproducible XcodeGen and build-setting draft |
| `Sources/VF2ROMImport.mm` | UIKit document-picker import and sandbox copy boundary |
| `Sources/VF2InputBridge.mm` | GameController and touch-action bridge into the existing `control_action` vocabulary |
| `../../src/host` | Existing SDL3 host, menu, ROM validation and persistence logic |
| `../../src/gpu/gpu3d_metal.mm` | Existing Metal 3D renderer; it must share SDL's Metal layer/device |

## Execution policy: interpreter only

The normal desktop startup compiler is `src/rec/i960_native.cpp`, which uses SLJIT to emit executable ARM64/x86-64 memory. iOS application processes must not create executable code at runtime. The port therefore sets `VF2_DISABLE_JIT=1` and `JIT=0`:

* `model2a` does not call `use_native()` at construction.
* `model2a::use_native()` is a safe false-returning stub.
* The existing MAME i960 interpreter (`third_party/mame/i960`) is selected because `i960_device` has no `runtime` attached.
* Desktop's runtime switch remains `--interp` / `VF2_INTERP=1`; iOS makes that selection compile-time mandatory. `src/rec/i960_rec.cpp` also refuses native regeneration in this mode.

This is functionally the project's existing reference path, but performance is unmeasured. Do not enable SLJIT or attempt entitlement-based JIT workarounds for a normal iOS distribution.

## Proposed ROM import flow

1. The app presents `UIDocumentPickerViewController` restricted to ZIP archives.
2. The selected URL is security-scoped only while importing. An `NSFileCoordinator` reads it.
3. The file is copied to a temporary file under `Library/Application Support/Virtua Fighter 2 Recomp/roms/`.
4. `romset::load` validates the expected VF2 2.1 file names and CRCs. Failure deletes the temporary file and shows its existing diagnostic.
5. A successful archive replaces only the sandboxed `vf2.zip`. Existing player saves remain separate in the app support directory.

The implementation deliberately does not bundle, download, share, or synthesize ROMs.

## Input design

`VF2InputBridge` translates `GCController` callbacks and a future UIKit overlay into `control_action` values already used by `src/host/controls.h`: direction, Guard, Punch, Kick, combinations, Start, Coin, and Menu. SDL3 continues to own the host event loop and window. The next integration step is to merge `VF2InputSnapshot` into the per-frame input sampling in `src/host/main.cpp`, so physical controllers and touch controls can coexist without inventing a second game-input format.

Recommended virtual layout: left-side four-way directional pad; right-side Guard/Punch/Kick triangle; top-right Start/Menu; optional combined buttons for accessibility. Touch controls must be rendered above the SDL content and respect safe-area insets, orientation, pause/menu state, and accessibility scaling.

## Rendering/window boundary

SDL3 should create the iOS window and own lifecycle events. Request its Metal renderer, retrieve the same `CAMetalLayer` with `SDL_GetRenderMetalLayer`, and pass it to the existing `gpu3d_create_metal_layer`; this is already the Apple path in `src/host/main.cpp`. Do not create a competing `MTKView` or Metal command queue. Start with the CPU renderer fallback enabled; enable GPU 3D only after device testing confirms the shared-layer path.

## Prerequisites before the first build (not performed in this change)

* macOS with current Xcode, iOS SDK, XcodeGen, and a device/simulator runtime.
* SDL3 built as an iOS XCFramework and FreeType packaged for `arm64` device and simulator slices (or equivalent Swift Package/CocoaPods integration).
* A bundle identifier, development team, provisioning profile, and a physical device for GameController/document-picker validation. Simulator results are insufficient for controller and Metal performance.
* App icon, launch/splash policy, privacy manifest, export-compliance review, and third-party license/attribution review.

## Verification plan (not run)

1. Generate the project from `project.yml`; inspect the resulting signing, architectures, framework and `JIT=0` settings.
2. Build/run on an arm64 device, import a known-valid user-owned archive, then confirm bad/missing CRC input leaves no archive behind.
3. Confirm app relaunch finds only the sandbox copy; revoke access to the source file and verify the game still launches.
4. Test GameController connect/disconnect and touch actions, including two simultaneous directions/buttons and focus loss.
5. Compare interpreter frames/audio/save behavior with the project's desktop `--interp` reference; test Metal GPU output against CPU fallback before enabling Auto/GPU.
6. Profile sustained play for thermal, memory, audio underruns, background/foreground recovery and orientation changes.

## Known risks

Interpreter performance may be below real-time; the SDL3 iOS packaging and SDL renderer/Metal-layer property names need confirmation against the selected SDL release; RmlUi's SDL backend needs touch/text-input checks; and the current desktop file-picker/UI messaging must be bypassed in favor of UIKit on iOS.
