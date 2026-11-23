# Makefile - Stunt Man Bob for the 48K Oric-1 / Atmos (docs/PLAN.md F0)
#   make              build/stuntbob.tap: release build for real hardware (+ .bin .map .lbl, size report)
#   make ORICUTRON=1  build/oricutron/stuntbob.tap: the build for Bob to test in Oricutron
#                     (noise an octave lower, DESIGN "Sound")
#   make test         both builds, emulator, native reference; runs tools/test_f0.py
#   make ref          build/ref/ref: native model with the per-step trace (tools/ref/ref.c)
#   make emu          build/emu/emu: headless emulator (tools/emu/emu.c)
#   make clean
# On Linux x86_64 the vendored cc65 2.19 is used (tools/vendor/cc65); elsewhere
# cc65 must be on PATH (macOS: brew install cc65).

VENDOR := $(CURDIR)/tools/vendor/cc65
ifeq ($(shell uname -s)-$(shell uname -m),Linux-x86_64)
export PATH      := $(VENDOR)/bin-linux-x86_64:$(PATH)
export CC65_HOME := $(VENDOR)/share
endif

ifeq ($(ORICUTRON),1)
B      := build/oricutron
AFLAGS := --cpu 6502 -D ORICUTRON=1
CDEFS  := -DORICUTRON=1
else
B      := build
AFLAGS := --cpu 6502
CDEFS  :=
endif

CFLAGS := -t none --cpu 6502 -O -Or -Cl -I src -I build/gen $(CDEFS)
LDCFG  := src/oric48.cfg
HOSTCC := cc
NAME   := stuntbob

OBJS := $(B)/crt0.o $(B)/main.o $(B)/model.o $(B)/trace.o $(B)/screen.o \
        $(B)/scene.o $(B)/system.o $(B)/keys.o $(B)/sound.o $(B)/draw.o $(B)/art.o
HDRS := $(wildcard src/*.h) build/gen/art.h

.PHONY: all test ref emu clean
.SECONDARY:
.SUFFIXES:

all: $(B)/$(NAME).tap

$(B)/%.o: src/%.s
	@mkdir -p $(B)
	ca65 $(AFLAGS) -o $@ $<

# pixel art, drawn as text in tools/mkart.py
build/gen/art.s build/gen/art.h: tools/mkart.py
	@mkdir -p build/gen
	python3 tools/mkart.py build/gen/art.s

$(B)/art.o: build/gen/art.s
	@mkdir -p $(B)
	ca65 $(AFLAGS) -o $@ $<

$(B)/%.s: src/%.c $(HDRS)
	@mkdir -p $(B)
	cc65 $(CFLAGS) -o $@ $<

$(B)/%.o: $(B)/%.s
	ca65 $(AFLAGS) -o $@ $<

$(B)/$(NAME).bin: $(OBJS) $(LDCFG)
	ld65 -C $(LDCFG) -m $(B)/$(NAME).map -Ln $(B)/$(NAME).lbl -o $@ $(OBJS) none.lib
	@python3 tools/size.py $(B)/$(NAME).map

$(B)/$(NAME).tap: $(B)/$(NAME).bin tools/mktap.py
	python3 tools/mktap.py --name STUNTBOB $< $@

ref: build/ref/ref
emu: build/emu/emu

build/ref/ref: tools/ref/ref.c src/model.c src/trace.c $(HDRS)
	@mkdir -p build/ref
	$(HOSTCC) -O2 -Wall -Wextra -I src -o $@ tools/ref/ref.c src/model.c src/trace.c

build/emu/emu: tools/emu/emu.c
	@mkdir -p build/emu
	$(HOSTCC) -O2 -Wall -o $@ $<

test:
	$(MAKE) build/$(NAME).tap
	$(MAKE) ORICUTRON=1 build/oricutron/$(NAME).tap
	$(MAKE) B=build/b3 CDEFS=-DBILLING_START=3 build/b3/$(NAME).tap
	$(MAKE) build/emu/emu build/ref/ref
	python3 tools/test_f0.py

clean:
	rm -rf build
