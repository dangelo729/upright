# upright — Makefile
# the host rig lives in extras/host; this forwards to it so `make test`,
# `make tool` and `make corpus` work from the repository root. the arduino
# build ignores this file.
#
#   make board    every example compiled for teensy41 by platformio. nothing
#                 is flashed.
#
# make board, how:
#   - `pio ci` builds one sketch against a library with no project file. it
#     copies each --lib directory whole into its build directory, and that
#     directory sits inside this repository (extras/host/build/pio, which
#     .gitignore covers): `--lib .` would copy the build into itself, with
#     .git and the host build. so the library is staged first as a consumer
#     gets it (src/ and the manifests) in extras/host/build/pio/lib, and that
#     copy is the --lib.
#   - --keep-build-dir keeps each example's firmware.elf, whose symbol table
#     gives the voices' size on the board; pio ci does not copy over a
#     library or sketch already there, so those two are removed first.
#   - the flags are the house's: -Wall -Wextra -fno-math-errno, and a
#     sketch's own from a line in it, "// make board: FLAGS" (a sketch's usb
#     type: -DUSB_MIDI_AUDIO_SERIAL, which the teensy platform takes in place
#     of its default USB_SERIAL). not -Werror:
#     the framework's own sources and headers (DMAChannel.h, FS.h, SD,
#     the audio library) raise warnings under -Wextra that are not ours to
#     fix. instead the build's output is searched for any warning whose file
#     is not under $(PIO_PKGS) (the library's sources and headers,
#     the sketch), and one fails the target.
#   - the end of each build is printed: teensy_size's flash and ram lines and
#     the largest objects in ram by the elf's symbol table.

PIO      ?= $(HOME)/.platformio/penv/bin/pio
PIO_PKGS ?= $(HOME)/.platformio/packages
NM       ?= $(PIO_PKGS)/toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-nm
PIO_DIR  := extras/host/build/pio
PIO_LIB  := $(PIO_DIR)/lib/Upright
EXAMPLES := $(sort $(wildcard examples/*/*.ino))
BOARD_FLAGS := -Wall -Wextra -fno-math-errno

.PHONY: all test tool corpus clean board

all test tool corpus clean:
	$(MAKE) -C extras/host $@

board:
	@set -e; \
	rm -rf $(PIO_DIR)/lib; \
	mkdir -p $(PIO_LIB); \
	cp -R src library.json library.properties keywords.txt $(PIO_LIB)/; \
	for ino in $(EXAMPLES); do \
		name=$$(basename $$ino .ino); \
		dir=$(PIO_DIR)/$$name; \
		log=$(PIO_DIR)/$$name.log; \
		own=$$(sed -n 's|^// make board: ||p' $$ino | head -1); \
		rm -rf $$dir/lib $$dir/src; \
		echo "== $$name ($$ino) for teensy41: $(BOARD_FLAGS) $$own"; \
		if ! $(PIO) ci --board teensy41 --lib $(PIO_LIB) --build-dir $$dir --keep-build-dir \
			--project-option="build_flags=$(BOARD_FLAGS) $$own" $$ino > $$log 2>&1; then \
			grep -E "error|warning" $$log | grep -v "$(PIO_PKGS)/" | head -40 || true; \
			tail -20 $$log; \
			echo "make board: $$name did not build (the whole log: $$log)"; \
			exit 1; \
		fi; \
		if grep -E "warning:" $$log | grep -v "$(PIO_PKGS)/"; then \
			echo "make board: $$name warns in our files (the whole log: $$log)"; \
			exit 1; \
		fi; \
		grep -E "teensy_size:|SUCCESS" $$log; \
		if [ -x "$(NM)" ]; then \
			echo "  the largest objects in ram (bytes, symbol):"; \
			$(NM) -C -S --size-sort -t d $$dir/.pio/build/teensy41/firmware.elf | \
				awk '$$3 ~ /^[bBdD]$$/ { n = $$4; for (i = 5; i <= NF; ++i) n = n " " $$i; printf "    %8d  %s\n", $$2, n }' | \
				tail -n 10; \
		fi; \
		echo "  0 warnings in our files"; \
	done; \
	echo "make board: every example built for teensy41, nothing flashed"
