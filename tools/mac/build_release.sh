#!/bin/sh
# The Mac release: vf2 for Apple silicon and Intel, macOS 11 and later, with SDL3 and FreeType
# built in (static, from build/deps: see tools/mac/deps.sh), joined into one universal program.
#   tools/mac/build_release.sh      ->  build/rel/vf2 (universal)
set -e
cd "$(dirname "$0")/../.."
DEPS=build/deps/mac
[ -f $DEPS/lib/libSDL3.a ] || tools/mac/deps.sh
FW="-Wl,-framework,CoreMedia -Wl,-framework,CoreVideo -Wl,-framework,Cocoa -Wl,-weak_framework,UniformTypeIdentifiers \
-Wl,-framework,IOKit -Wl,-framework,ForceFeedback -Wl,-framework,Carbon -Wl,-framework,CoreAudio -Wl,-framework,AudioToolbox \
-Wl,-framework,AVFoundation -Wl,-framework,Foundation -Wl,-framework,GameController -Wl,-framework,Metal \
-Wl,-framework,QuartzCore -Wl,-weak_framework,CoreHaptics"
for A in arm64 x86_64; do
	make -s -j10 OBJ=build/rel/$A/obj BIN=build/rel/$A/bin \
		CXX="clang++ -arch $A -mmacosx-version-min=11.0" \
		SDL_CFLAGS="-I$DEPS/include" SDL_LIBS="$DEPS/lib/libSDL3.a $FW -liconv" \
		FT_CFLAGS="-I$DEPS/include/freetype2" FT_LIBS="$DEPS/lib/libfreetype.a" \
		build/rel/$A/bin/vf2
done
lipo -create build/rel/arm64/bin/vf2 build/rel/x86_64/bin/vf2 -output build/rel/vf2
lipo -archs build/rel/vf2
