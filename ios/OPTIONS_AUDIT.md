# iOS options audit — build 7

The user reported that build 5 fixes cold-launch picture clipping/UI enlargement.
This audit covers every menu category. “Source checked” means callbacks, value
ranges and their application/save paths were inspected; it does not mean that
actual game behavior was tested with a ROM. No game archive or extracted data
was obtained. The following device-specific behavior still needs an iPhone run.

| Category / item | Application and persistence checked | Changes / validation |
| --- | --- | --- |
| Main: Resume / Back | Menu close/pop and input/audio state reset | Source checked |
| Main: Quit | Stops loop, joins workers, saves NVRAM/settings, shuts down SDL and destroys host locals, then exits the iOS process | Fixed black screen: SDL UIKit intentionally keeps UIApplication alive after SDL_main returns. Explicit Quit now exits only after normal cleanup; device confirmation remains |
| Display: Resolution | 1x–6x and native target; pending work finishes before recreation; `ios_resolution` saved | Existing actual Metal target/switch/recreation tests |
| Display: GPU quality | Changes Metal Exact/Fast immediately; `gpu_quality` saved | Synthetic geometry tested in both modes |
| Display: Background filter | Backdrop flag, redraw, linear/nearest texture sampling; `background_filter` saved | Removed erroneous `scale > 1` gate at native output; checkerboard Off/Smooth/Off pixel test |
| Display: Smooth motion | Enables interpolated pictures; host does not reduce quality/scale automatically on iOS; `smooth_motion` saved | Added missing high-refresh iPhone plist opt-in; pinned SDL requests maximum refresh. Actual 120 fps is not measured by CI |
| Display: Brightness | Completed game texture modulation, 50–200%; `brightness` saved | Existing 100/150/200/100% exact RGB tests; UI not modulated |
| Display: Screen filter | Off/Scanlines/CRT drawn after the game and before UI; `screen_filter` saved | Full-row phase normalization; tests require visible scanline contrast/CRT RGB stripe changes and restored Off pixels |
| Display: Show FPS | Measures shown pictures, updates overlay, clears it when disabled; `show_fps` saved | iOS placement now follows safe-area insets at upper left. Physical notch/touch layout remains device validation |
| Sound: Volume | `m.set_level`, 0–100%; `volume` saved | Source checked; listening test required |
| Sound: Music | `m.set_volumes`, 0–100%; `music_volume` saved | Source checked; listening test required |
| Sound: Effects and voices | `m.set_volumes`, 0–100%; `effects_volume` saved | Source checked; listening test required |
| Game: Play style | PC/Arcade switch changes free-play policy and restarts; `arcade_mode` and original coin setting saved | Source checked; actual credits/start behavior requires game run |
| Game: Reset game | Finishes async work, closes menu, boots again, preserves rankings/NVRAM | Source checked; actual reset requires game run |
| Arcade: Difficulty | Easy–Hardest; updates energy/ring defaults and CRC-protected assignment bytes | Source checked; NVRAM saved and game restarted after changed assignments |
| Arcade: Ring size | Default/Small/Medium/Large/Extra large; `ring_size` saved; reapplied at boot | Source checked |
| Arcade: Rounds to win (vs CPU / vs player) | Range 2–5 in game assignment bytes | Source checked; assignment CRC and restart path |
| Arcade: Continue / Demo sound | Toggle assignment bits with CRC | Source checked; restart path |
| Arcade: After a versus match | Winner plays on / Both stop in assignment bits | Source checked; restart path |
| Arcade: Version | Original / 2.1 in assignment bits | Source checked; restart path |
| Arcade: Service menu | Closes host menu and queues test input after boot; Down/Start operate service inputs | Source checked; game-dependent runtime test required |
| Controls: Player 1 / Player 2 pad | Auto, connected GUID or None; both choices saved | Fixed iOS gameplay bypass of selected SDL controllers; needs physical controller validation |
| Controls: Keyboard (both players, every action) | Captures scancodes, removes duplicates, supports clear, saves per-action binding | Round-trip test covers every action for both players; external keyboard required |
| Controls: Pad buttons (both players, every listed action) | Captures SDL buttons/triggers, removes duplicate action binding, saves per action | Fixed iOS hard-coded physical mapping bypass; round-trip test covers all actions including Start/Coin/Menu and trigger |
| Controls: Reset to defaults | Restores one player's key/button defaults, resolves other player's key conflicts, reopens pads | Source checked |
| Controls: Swap pads | Swaps chosen controllers; identical-GUID controllers use swap flag | Swap flag now saved instead of lost on restart; physical two-controller test required |
| Cheats: Secret character | Off/Dural/MUE/KOJAC, `secret_character` saved; applied at restart | Source checked; actual game character behavior requires ROM/device |
| Touch: directions/G/P/K/Coin/Start/Menu | Direct actions supplement shared controller path; hidden when controller connected | No fixed physical input is injected; held touch Start now reaches cabinet start bit |
| Storage / lifecycle | Changed settings saved on menu close, periodically, background and exit; game assignments/rankings in separate NVRAM files | Source checked; no reset/deletion of player data |

Desktop-only Render scale/Antialiasing/3D renderer/Picture shape/Fullscreen and
Windows graphics-driver choices remain hidden on iOS. Resolution replaces the
integer-scale selector; CPU execution is interpreter-only regardless of render
quality or refresh rate. Physical controllers now use SDL's GameController
backend once, so remaps/selection/None/Swap do not compete with a fixed GC mapping.

## Build 8: original choices and iPhone exception

Resolution keeps the original 1x, 2x, 3x, 4x, 5x, 6x order and appends
2868 x 1320 (iPhone). Original scales honor all original Picture shape choices
(Fit the window, 4:3, 16:10, 16:9, 21:9). Only the iPhone option ignores
that saved choice and fills the entire screen. Returning to an original scale
restores the saved shape; selecting iPhone does not overwrite it.
