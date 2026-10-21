# Personal iOS port workspace

- Work only in `ajellia81-cmd/vf2-recomp`, on `ios-port`. Keep
  `mr-finster/vf2-recomp` as read-only upstream. Never push upstream or force-push.
- Make small reviewable commits and push each completed change to the personal branch.
- iOS is interpreter-only: preserve `VF2_DISABLE_JIT=1`, exclude SLJIT and
  `i960_native.cpp`, and use `i960_native_stub.cpp`. Do not enable JIT entitlements.
- Never obtain, download, upload, commit, synthesize, or distribute ROMs, extracted
  game data, or ROM-generated source. A player imports their own archive on-device.
- Do not archive/export/package an IPA or publish releases without a new explicit
  request. Unsigned app compilation for verification is allowed; do not upload it.
- No dependency on the user's Windows computer. Use remote Linux checks and macOS
  GitHub Actions for Apple SDK compilation. Do not claim a Codex Cloud environment
  exists unless a published environment/task has actually been verified.
- `ios/project.yml` is the XcodeGen source of truth. Do not commit generated Xcode
  projects, XCFrameworks, signing secrets, provisioning profiles, or build outputs.
- Use Korean progress/final messages. Distinguish source inspection, Linux checks,
  Apple SDK compilation, simulator runs and physical-device tests. Compilation
  alone does not establish working ROM import, controls, graphics or performance.
