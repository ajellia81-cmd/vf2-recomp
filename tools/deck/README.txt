VIRTUA FIGHTER 2 RECOMP for Linux / Steam Deck (preview)
========================================================

An unofficial fan project, not made, approved or supported by SEGA. Run it at your own risk.
No game files are included: you need your own MAME "vf2" set (vf2.zip, Version 2.1).

The game's own program is recompiled from your ROM to native code as it starts.

For the Steam Deck and other x86-64 Linux PCs: it's built against Valve's Steam Runtime, the
libraries every Steam install has. On another Linux PC, unpack the VirtuaFighter2 folder anywhere,
put your vf2.zip in its roms folder and run VirtuaFighter2.sh (or add it to Steam as below).

INSTALL ON THE STEAM DECK
1. Copy this VirtuaFighter2 folder to the Deck, e.g. /home/deck/Games/VirtuaFighter2
   (a USB stick, or Desktop Mode's file manager).
2. Put your vf2.zip in the VirtuaFighter2/roms folder.
3. Desktop Mode: Steam > Games > Add a Non-Steam Game to My Library > Browse, pick
   VirtuaFighter2.sh (set the file type filter to "All files"), Add Selected Programs.
4. Back in Game Mode, launch it from your Library with the default Gamepad layout.

It starts fullscreen at the Deck's 16:10, at render scale 1x (the arcade's own resolution). Try 2x
in the menu's Display page (or View + LB): if the Deck can't keep up, the game lowers it again for
that session. Show FPS (Display) puts the frame rate in a corner: the arcade's is 57.5.

CONTROLS
  D-pad / left stick   move            X  guard     A  punch     B  kick
  Y   guard + punch (throws)           RB punch + kick           LB guard + kick
  RT  guard + punch + kick             Start  start
  View (tap)  the menu: display, sound, game, arcade settings, controls and quit
  View + LB / RB   render scale / picture shape

Settings, rankings, screenshots and saves are kept in this folder. See LICENSES.txt for the
software it uses: MAME's Model 2 work (the i960 and 68000 CPU cores, the TGP, the SCSP sound chip,
the 3D renderer and the board), SDL, RmlUi, FreeType, zlib and stb_image.
