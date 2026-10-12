# Virtua Fighter 2 (Model 2A) recomp: the SDL3 game (build/bin/vf2) and headless tools.
# Mac: brew install sdl3 freetype.
# ROM-derived data lives in build/ and is never committed.
# The downloads: tools/mac/package.sh, tools/win/build.sh (MinGW-w64), tools/deck/build.sh (Steam
# Runtime container), each into build/ with tools/release/licenses.sh's LICENSES.txt.
CXX ?= clang++
CXXFLAGS ?= -std=c++20 -O2 -g -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function
CXXFLAGS += -Isrc/core/shim_include $(EXTRA_CFLAGS)
LDLIBS ?= -lz
SDL_CFLAGS ?= $(shell pkg-config --cflags sdl3 2>/dev/null || echo -I/opt/homebrew/include)
SDL_LIBS ?= $(shell pkg-config --libs sdl3 2>/dev/null || echo -L/opt/homebrew/lib -lSDL3)
FT_CFLAGS ?= $(shell pkg-config --cflags freetype2 2>/dev/null || echo -I/opt/homebrew/include/freetype2)
FT_LIBS ?= $(shell pkg-config --libs freetype2 2>/dev/null || echo -L/opt/homebrew/lib -lfreetype)

OBJ ?= build/obj
BIN ?= build/bin
objs = $(patsubst %.cpp,$(OBJ)/%.o,$(1))
all: $(BIN)/vf2 $(BIN)/vf2romset $(BIN)/vf2run $(BIN)/vf2dis $(BIN)/vf2thunks

# the version and date shown in the menu: the git tag (or tag-commits-hash between releases, or
# the commit) and the date of the last commit. Builds without git are handed them in the
# environment. Rewritten only when they change, so main is rebuilt for a new version only.
VF2_VERSION ?= $(shell git describe --tags --always 2>/dev/null || echo dev)
VF2_DATE ?= $(shell git log -1 --format=%cd --date=format:'%b %-d %Y' 2>/dev/null)
$(OBJ)/version.h: FORCE
	@mkdir -p $(OBJ)
	@printf '#define VF2_VERSION "%s"\n#define VF2_DATE "%s"\n' "$(VF2_VERSION)" "$(VF2_DATE)" > $@.tmp
	@cmp -s $@.tmp $@ && rm -f $@.tmp || mv $@.tmp $@
FORCE:

# MAME's i960 core and disassembler (third_party/mame), on our shim, with the recompiled code's
# runtime (src/rec: the code found with the address map, src/rec/vf2_map.cpp, made native at start-up)
CORE = third_party/mame/i960/i960.cpp third_party/mame/i960/i960dis.cpp src/core/mame_shim.cpp src/core/mb86233.cpp \
	src/core/i960_device.cpp src/rec/i960_rec.cpp src/rec/i960_analyse.cpp src/rec/i960_native.cpp src/rec/vf2_map.cpp
MACHINE = src/machine/model2a.cpp src/machine/model2_video.cpp src/machine/model2_smooth.cpp src/machine/scsp.cpp src/machine/scspdsp.cpp
# MAME's 68000 (the sound board's CPU): the generated instruction handlers, unchanged
M68000 = third_party/mame/m68000/m68000.cpp third_party/mame/m68000/m68000-decode.cpp \
	third_party/mame/m68000/m68000-sdf.cpp third_party/mame/m68000/m68000-sdp.cpp
HOST = src/host/romset.cpp src/host/input_script.cpp


# SLJIT (BSD-2): the native code made at start-up. C, built by the C++ compiler's driver (-x c), so
# that the builds that set only CXX (a cross-compiler, -arch) build it for the same target.
SLJIT_OBJ = $(OBJ)/third_party/sljit/sljitLir.o
$(SLJIT_OBJ): third_party/sljit/sljitLir.c
	@mkdir -p $(dir $@)
	$(CXX) -x c -O2 -w -DSLJIT_CONFIG_AUTO=1 -c $< -o $@

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# The renderer's float arithmetic: clang on Apple silicon fuses a * b + c into one rounding (fused
# multiply-add), as MAME built there does (the reference pictures); x86-64 builds can't without
# requiring a CPU with FMA, so they round twice, as MAME built for x86-64 does, and a few texels a
# frame differ from the Apple silicon reference (everything else is the same: CPU, RAM, sound).
# RENDER_FLAGS="-mfma" (clang) or "-mfma -ffp-contract=fast" (GCC) gives an x86-64 build the
# reference's pictures exactly, for checking; it then needs FMA (Intel 2013 Haswell, AMD 2012 on).
$(OBJ)/src/machine/model2_video.o $(OBJ)/src/machine/model2_smooth.o: CXXFLAGS += $(RENDER_FLAGS)

# RmlUi (MIT, third_party/rmlui): the in-game menu. Its core and the SDL backend, built with FreeType.
RML_SRC = $(wildcard third_party/rmlui/Source/Core/*.cpp third_party/rmlui/Source/Core/*/*.cpp) \
	third_party/rmlui/Backends/RmlUi_Platform_SDL.cpp third_party/rmlui/Backends/RmlUi_Renderer_SDL.cpp
RML_CFLAGS = -DRMLUI_STATIC_LIB -DRMLUI_FONT_ENGINE_FREETYPE -DRMLUI_SDL_VERSION_MAJOR=3 -Ithird_party/rmlui/Include -Ithird_party/rmlui/Backends
$(OBJ)/third_party/rmlui/%.o: CXXFLAGS += $(RML_CFLAGS) $(FT_CFLAGS) $(SDL_CFLAGS) -w
$(OBJ)/src/host/main.o $(OBJ)/src/host/menu.o: CXXFLAGS += $(SDL_CFLAGS) $(RML_CFLAGS)
$(OBJ)/src/host/main.o: $(OBJ)/version.h
$(OBJ)/src/host/main.o: CXXFLAGS += -I$(OBJ)

$(OBJ)/src/host/stb_image_impl.o: CXXFLAGS += -w

# No recompiled C++ in the game or the tools by default (src/rec/gen_none.cpp): it would be made
# from the ROM. `make rec` builds the C++ recompile (build/gen, from the player's roms/vf2.zip)
# into build/bin/vf2lock and build/bin/vf2run-rec, for the lockstep check.
GEN_NONE = src/rec/gen_none.cpp

$(BIN)/vf2: $(call objs,src/host/main.cpp src/host/menu.cpp src/host/stb_image_impl.cpp $(MACHINE) $(HOST) $(CORE) $(GEN_NONE) $(M68000) $(RML_SRC)) $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS) $(SDL_LIBS) $(FT_LIBS)

$(BIN)/vf2romset: $(call objs,tools/vf2romset.cpp $(HOST))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(BIN)/vf2dis: $(call objs,tools/vf2dis.cpp $(HOST) $(CORE) $(GEN_NONE)) $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(BIN)/vf2run: $(call objs,tools/vf2run.cpp src/host/stb_image_impl.cpp $(MACHINE) $(HOST) $(CORE) $(GEN_NONE) $(M68000)) $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -rf $(OBJ) $(BIN)

.PHONY: all clean FORCE
-include $(shell find $(OBJ) -name '*.d' 2>/dev/null)

$(BIN)/vf2thunks: $(call objs,tools/vf2thunks.cpp $(HOST))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

# ---- the C++ recompile (ROM-derived: build/gen only, never committed) ----
$(BIN)/vf2recomp: $(call objs,tools/recomp/vf2recomp.cpp src/rec/i960_analyse.cpp src/core/mame_shim.cpp $(HOST))
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

build/gen/vf2_rec.cpp: $(BIN)/vf2recomp src/rec/vf2_map.cpp roms/vf2.zip
	@mkdir -p $(dir $@)
	$(BIN)/vf2recomp roms/vf2.zip cpp src/rec/vf2_map.cpp $@

# The address map, from coverage runs (vf2run --cov build/cov/c_NAME.txt ...): `make map` after new ones
map: $(BIN)/vf2recomp
	$(BIN)/vf2recomp roms/vf2.zip map src/rec/vf2_map.cpp build/cov/c_*.txt

# (clang only: the functions go on to each other with guaranteed tail calls)
$(OBJ)/build/gen/vf2_rec.o: build/gen/vf2_rec.cpp src/rec/i960_gen.h src/rec/i960_ops.h
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -O2 -ffp-contract=off -Isrc/rec -MMD -MP -c $< -o $@

$(BIN)/vf2lock: $(call objs,tools/vf2lock.cpp src/host/stb_image_impl.cpp $(MACHINE) $(HOST) $(CORE) $(M68000)) $(OBJ)/build/gen/vf2_rec.o $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

$(BIN)/vf2run-rec: $(call objs,tools/vf2run.cpp src/host/stb_image_impl.cpp $(MACHINE) $(HOST) $(CORE) $(M68000)) $(OBJ)/build/gen/vf2_rec.o $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)

rec: $(BIN)/vf2lock $(BIN)/vf2run-rec
.PHONY: rec map

# ---- the twin-board checks of the pictures (tools/checks/README.txt) ----
CHECKS = onex spipe rdtest partscmp
$(BIN)/check-%: $(call objs,tools/checks/%.cpp src/host/stb_image_impl.cpp $(MACHINE) $(HOST) $(CORE) $(GEN_NONE) $(M68000)) $(SLJIT_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDLIBS)
checks: $(addprefix $(BIN)/check-,$(CHECKS))
.PHONY: checks
