#!/bin/sh
# Launcher for Steam: runs the game from its own folder (the saves and settings stay in it).
cd "$(dirname "$0")" && exec ./vf2 "$@"
