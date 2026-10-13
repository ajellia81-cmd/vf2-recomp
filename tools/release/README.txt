VIRTUA FIGHTER 2 RECOMP (preview)
=================================

An unofficial fan project, not made, approved or supported by SEGA. Run it at your own risk.

Virtua Fighter 2 (Sega Model 2, 1994) as a native program for your computer: the game's own
program is recompiled from your ROM to native code as it starts, and the rest of the arcade board
(the TGP geometry processor, the 3D renderer and the sound board) runs on code from MAME, inside a
PC host with widescreen, higher resolutions, a menu and gamepads. No game files are
included: you need your own MAME "vf2" set.


WHAT YOU NEED
- A Mac with macOS 11 (Big Sur) or later, Apple silicon or Intel.
- Your own copy of the game as MAME's "vf2" set (vf2.zip, Version 2.1). That's all it needs.


FIRST LAUNCH
1. Unzip and move "Virtua Fighter 2 Recomp" to Applications (or anywhere).
2. The app isn't signed, so the first time macOS won't open it. Double-click it once, close the
   message, then go to System Settings > Privacy & Security, scroll down and click Open Anyway.
   (On macOS 14 and earlier you can instead right-click the app, choose Open, then Open again.)
3. It asks for your vf2.zip. Choose it. It's copied into
   ~/Library/Application Support/Virtua Fighter 2 Recomp/roms, so you only do this once.

Prefer to keep everything in the game's folder (portable)? Put a file named portable.txt next to
the app, in the same folder (an empty one will do). The game then keeps everything in that folder
and nothing in ~/Library/Application Support: settings, saves, screenshots, log.txt, and your
vf2.zip, which it asks for once and copies into a "roms" folder there. (An app that hasn't been
moved since it was downloaded runs from a hidden copy that can't see the file: move it with the
Finder first.)

Keep your ROMs in a folder of your own? Add a line to settings.ini (in the data folder above, or
beside the app in portable mode):  roms_folder = ~/ROMs/vf2
or start it from Terminal:  open "Virtua Fighter 2 Recomp.app" --args --roms ~/ROMs/vf2
(or set VF2_ROMS). The game looks there first for vf2.zip and only reads it; log.txt says which
folder it used, or where it looked when the zip isn't there (it then looks in the usual place).


CONTROLS (change them in the menu: Esc, or View/Select on a pad)
Keyboard, player 1:  arrows move   Z guard   X punch   C kick   1 start
Keyboard, player 2:  I J K L move  B guard   N punch   M kick   2 start
Pads (Xbox layout):  d-pad/stick move   X guard   A punch   B kick   Y guard+punch (throw)
                     RB punch+kick   LB guard+kick   RT all three   Start start
Esc menu   P pause   F3 reset   F5 render scale   F6 picture shape   F9 frame timings
F11 fullscreen   F12 screenshot

8BitDo pads on a Mac: use Switch (S) mode.


OPTIONS
Display: render scale (up to 6x), antialiasing, picture shape (fits the window: widescreen and
ultrawide), background and screen filters (scanlines, CRT), Show FPS, fullscreen. Sound: volume,
music, effects and voices. Game: play style (PC: free play, no boot screen / arcade: coins and
credits), reset. Arcade settings: difficulty, ring size, rounds, continues and the arcade's own
service menu. Controls: each player's pad, keys and buttons.
3D renderer: Auto (the default) draws the 3D on the graphics card, the same picture as the CPU,
unless it is clearly slower on your machine; GPU or CPU to choose yourself. If the GPU ever
draws differently or fails, the game uses the CPU for that run (log.txt says so).

If your Mac can't keep up at the render scale, the game lowers it for you (for that session).


YOUR FILES
Settings, saves (rankings, the arcade settings), screenshots and your ROMs are in
~/Library/Application Support/Virtua Fighter 2 Recomp (or beside the app, with a portable.txt
there). Put your own select.png in its
"backgrounds" folder to have it used for the player select's widescreen sides.


CREDITS
Built on MAME's work on the Model 2 (the i960 and 68000 CPU cores, the TGP, the SCSP sound chip,
the 3D renderer and the board), SDL, RmlUi, FreeType, zlib and stb_image: see LICENSES.txt. The
port was done with Claude Opus 5.5.
