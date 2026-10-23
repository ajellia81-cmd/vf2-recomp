# Personal haptics build 10 validation

- Source: 499fc02103cabc4fe116b9a06f87f3e716cf6b6c
- Workflow: https://github.com/ajellia81-cmd/vf2-recomp/actions/runs/39939595022
- Output: VirtuaFighter2-build10-unsigned.ipa
- Size: 6,144,080 bytes
- SHA-256: 4483c78ce031eeedca077636368e1c15022f58f4d184530d05233870e2c1d2b0

Apple CI passed interpreter-only project verification, iPhone Release compilation,
synthetic Metal rasterization/composition/display/controls checks, combat haptics
tracker/adapter checks and touch Menu checks. Two iOS Simulator import XCTest cases
passed with zero failures. Packaging checks passed without signing, archive/export
or ROM data.

Downloaded artifact SHA-256 and IPA SHA-256 match GitHub's artifact digest and
runner output respectively. ZIP integrity, bundle/build/source metadata, interpreter
policy and absence of game archives were checked. The executable contains the
Core Haptics backend and the vibration controls.

Linux synthetic tracker/adapter tests passed with C++20, -Wall -Wextra -Werror.
An additional temporary SDL mock harness checked assigned-controller routing, phone
fallback after failed rumble, Both/zero strength, menu cancellation, resume baseline,
controller identity changes and disable. This verifies logic, not physical hardware.

See HAPTICS.md for behavior and event limitations. No physical iPhone/controller
haptic test or gameplay test was performed for this new build. Existing user reports
about earlier builds do not establish this build's tactile feel.
