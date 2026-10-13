VIRTUA FIGHTER 2 RECOMP for Linux / Steam Deck (preview)
========================================================

An unofficial fan project, not made, approved or supported by SEGA. Run it at your own risk.
No game files are included: you need your own MAME "vf2" set (vf2.zip, Version 2.1).

The game's own program is recompiled from your ROM to native code as it starts.

For the Steam Deck and other x86-64 Linux PCs: it's built against Valve's Steam Runtime, the
libraries every Steam install has. On another Linux PC, unpack the VirtuaFighter2 folder anywhere,
put your vf2.zip in its roms folder and run VirtuaFighter2.sh (or add it to Steam as below).

INSTALL ON THE STEAM DECK
1. Copy the download to the Deck (e.g. Downloads) and unpack it there: in Desktop Mode, right-click
   it in the file manager > Extract > Extract archive here. (Unpacking on Windows, or copying the
   folder through a USB stick, can lose the files' permissions, and then nothing launches.)
2. Put your vf2.zip in the VirtuaFighter2/roms folder.
3. Desktop Mode: Steam > Games > Add a Non-Steam Game to My Library > Browse, set the file type
   filter to "All files", pick the vf2 file in the VirtuaFighter2 folder, Add Selected Programs.
   Then in its Properties: Start In is the VirtuaFighter2 folder (e.g. /home/deck/Downloads/
   VirtuaFighter2/), and under Compatibility, "Force the use of a specific Steam Play
   compatibility tool" is off (it's a native Linux game: Proton stops it launching).
4. Back in Game Mode, launch it from your Library with the default Gamepad layout.

It starts fullscreen at the Deck's 16:10, at render scale 1x (the arcade's own resolution). Try 2x
in the menu's Display page (or View + LB): if the Deck can't keep up, the game lowers it again for
that session. Show FPS (Display) puts the frame rate in a corner: the arcade's is 57.5.

YOUR OWN ROM FOLDER (Batocera, a ROMs folder an update won't touch)
To keep vf2.zip somewhere else, tell the game where it is, in any of these ways (the first one given wins):
  - on the command line: ./vf2 --roms /userdata/roms/ports/vf2   (or --roms=/userdata/roms/ports/vf2);
    in Steam: the game's Properties > Launch Options: --roms "/home/deck/ROMs/vf2"
  - the environment variable VF2_ROMS=/userdata/roms/ports/vf2
  - a line in settings.ini (in this folder):  roms_folder = /userdata/roms/ports/vf2
    (~ is your home folder; a relative path is from settings.ini's folder)
For example, a Batocera port launcher, /userdata/roms/ports/VirtuaFighter2.sh:
  #!/bin/bash
  exec /userdata/system/VirtuaFighter2/VirtuaFighter2.sh --roms /userdata/roms/ports/vf2
The game only reads that folder (log.txt says which folder it used). If vf2.zip isn't there, log.txt says
where it looked and the game uses the roms folder here as before; a set you then choose with the
file picker is copied into your ROM folder, so the next start finds it there.

CONTROLS
  D-pad / left stick   move            X  guard     A  punch     B  kick
  Y   guard + punch (throws)           RB punch + kick           LB guard + kick
  RT  guard + punch + kick             Start  start
  View (tap)  the menu: display, sound, game, arcade settings, controls and quit
  View + LB / RB   render scale / picture shape

Display > 3D renderer: Auto (the default) draws the 3D on the GPU (Vulkan), the same picture as
the CPU, unless it is clearly slower; GPU or CPU to choose yourself.

Settings, rankings, screenshots and saves are kept in this folder, and nothing anywhere else (a
portable.txt here keeps it that way even without the roms folder). See LICENSES.txt for the
software it uses: MAME's Model 2 work (the i960 and 68000 CPU cores, the TGP, the SCSP sound chip,
the 3D renderer and the board), SDL, RmlUi, FreeType, zlib and stb_image.
