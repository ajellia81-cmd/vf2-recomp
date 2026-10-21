# Local iOS build dependencies

Place these unmodified, redistributable development dependencies here before generating the Xcode project:

* `SDL3.xcframework` with iPhoneOS arm64 and iPhoneSimulator arm64 slices.
* `FreeType.xcframework` with the same slices, exposing headers under the framework's `Headers/` directory.

They are intentionally ignored by Git. Do not put ROMs, generated ROM code, or game assets in this directory. The project links zlib from the iOS SDK.

The selected SDL3 headers must provide the APIs used by this source tree, including SDL dialogs, gamepads, renderer Metal-layer access, and SDL GPU declarations. Confirm that at the first device build rather than silently substituting a different SDL major version.
