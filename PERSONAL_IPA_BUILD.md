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
