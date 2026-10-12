# Virtua Fighter 2 Recomp

Virtua Fighter 2 (SEGA, arcade, Model 2, 1994) as a native PC game. The game's own program, for
the board's Intel i960 CPU, is recompiled to native code (ARM64 or x86-64) from your ROM each time
it starts, and checked instruction for instruction, cycle for cycle against MAME's i960
interpreter; the rest of the Model 2A board (the TGP geometry processor, the geometrizer and
polygon renderer, the System 24 tile layers, and the sound board's 68000 and SCSP) runs on code
from MAME, inside an SDL 3 host. Render scales up to 6x, widescreen and ultrawide, smooth motion at
your screen's refresh rate, a menu and gamepads, and the arcade's frames and sound as MAME makes
them at 1x (pixel for pixel on Apple silicon; on x86-64 a few texels a frame round differently, the
renderer's multiply-adds not fused). Mac, Windows and Linux / Steam Deck.

**Downloads and more:** https://vf2-recomp.pages.dev/

An unofficial fan project, not made, approved or supported by SEGA, and not affiliated with it.

## Bring your own MAME `vf2` set

Nothing from the game is in this repository: no ROMs and nothing made from them. You need your own
dump of the game as MAME's `vf2` set (`vf2.zip`, Version 2.1). The game asks for it the first time
it starts.

## Build

Needs a C++20 compiler, zlib, SDL 3 and FreeType (Mac: `brew install sdl3 freetype`).

```
make                    # build/bin/vf2, and the headless tools
build/bin/vf2 path/to/vf2.zip
```

The release packages: `tools/mac/package.sh` (Mac), `tools/win/build.sh` (Windows, cross-compiled
with MinGW-w64), `tools/deck/build.sh` (Linux / Steam Deck: x86-64, in Valve's Steam Runtime
"sniper" SDK container). `tools/release/licenses.sh` gathers the third-party notices they carry.

`build/bin/vf2run` runs the board headless (pictures, RAM dumps, raw sound, an input script such as
`tools/scripts/start_game.txt`); `tools/compare.py` and `tools/sound_compare.py` compare its output
with MAME's.

The recompiler (`src/rec`) finds the program's code with an address map (`src/rec/vf2_map.cpp`:
where the functions start and where the indirect jumps lead, no bytes from the ROM), made from
coverage runs (`vf2run --cov`, then `make map`). `make rec` also recompiles it to C++ (into
`build/`, from your `roms/vf2.zip`: never committed) for `build/bin/vf2lock`, which runs the
interpreter and the recompiled code side by side and compares the CPU's whole state, the clocks,
the RAMs and the pictures as they go. `vf2 --interp` and `vf2run --interp` run the interpreter.

## Layout

| Path | What |
| --- | --- |
| `src/core` | the Fujitsu MB86233 (TGP) core, from MAME, and the shim MAME's CPU cores compile against |
| `src/rec` | the i960 recompiler: code discovery, native code (SLJIT) at start-up, the C++ build's, the runtime |
| `src/machine` | Model 2A: memory map, interrupts and timers, the TGP glue, geometrizer and renderer, the tile chip, the sound board, smooth motion |
| `src/host` | the SDL 3 host: window, input, pacing, scene sync, the GPU compositing, the RmlUi menu (`assets/ui`) |
| `third_party` | MAME's i960 and 68000 cores, SLJIT, RmlUi, stb_image |
| `tools` | the headless runner, disassembler, comparisons, the packaging scripts |

## How it's made

Made with the help of AI coding tools. Every build is checked against the arcade: the recompiled i960 program runs side by side with MAME's interpreter, and the pictures, RAM and sound are compared frame by frame with MAME's.

## Credits

Built on MAME's work on the Model 2: the i960 core and disassembler (Farfetch'd, R. Belmont), the
68000 core and the TGP (Olivier Galibert), the Model 2 driver and video (R. Belmont, Olivier
Galibert, ElSemi, Angelo Salese, Matthew Daniels), the System 24 tile chip (Olivier Galibert), the
SCSP and its DSP (ElSemi, R. Belmont), the polygon scan converter (Ville Linde, Aaron Giles) and
more (third_party/mame/README.md lists every file and its authors), all BSD-3-Clause. Also RmlUi
(MIT), stb_image (Sean Barrett, public domain / MIT), SDL 3 (zlib), FreeType (FreeType License),
zlib, and the Barlow Condensed font (SIL Open Font License).

Virtua Fighter 2 is SEGA's. This project is not affiliated with or endorsed by SEGA.

## Licence

MIT for this project's own code: see LICENSE. Code from MAME keeps its BSD-3-Clause licence and
notices; the rest of `third_party/` keeps its own, and the Barlow Condensed font the SIL Open Font
License. The art, the player select's painted sides (`assets/backgrounds/select.png`) and the app
icon (`assets/icon/`, drawn by `tools/release/make_icon.py`), is by Mr Finster under
[CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
