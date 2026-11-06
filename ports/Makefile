# The SDK, and programs built with it for A/UX 3: m68k-elf-gcc against our headers,
# linked against A/UX's shared C library and libaux. Written for macOS's make 3.81.
#
#   make                  the SDK and its test programs
#   make test-aux         the test programs, in A/UX in QEMU
#   make port-NAME        ports/NAME's package, in build/packages/
#   make test-port-NAME   that, then installed with pkg and tested in A/UX
#   make clean            all of build/ (downloads in dist/ are kept)

CC = m68k-elf-gcc
AR = m68k-elf-ar
PYTHON = python3 -I
GCC_INCLUDE := $(shell $(CC) -print-file-name=include)
WARN = -Wall -Wextra -Wno-unused-parameter
CFLAGS = -m68020 -m68881 -Os -fno-asynchronous-unwind-tables $(WARN) \
	-nostdinc -isystem sdk/include -isystem $(GCC_INCLUDE) -MMD -MP
LDFLAGS = -nostdlib -static -T sdk/lib/aux.ld -Lsdk/lib -Wl,--build-id=none \
	-Wl,--no-warn-rwx-segments

# For tests in QEMU: a stock A/UX 3.1 disk image (never changed: tests boot a clone), a
# Quadra 800 ROM, aux-3-patches, whose tools read and write A/UX disks, and aux-3-pkg,
# whose tools/aux-run.py runs the tests. Set AUX_DISK and AUX_ROM in local.mk (ignored
# by git) or on make's command line.
-include local.mk
AUX_PATCHES ?= ../aux-3-patches
AUX_PKG ?= ../aux-3-pkg
export AUX_DISK AUX_ROM AUX_PATCHES AUX_PKG

CRT0 = build/sdk/crt/crt0.o
LIBAUX_SRC = init stdio stdio-extra printf ctype malloc malloc-a0 exit string stdlib \
	signal unistd locale time env sys setjmp pwd ssp mman
LIBAUX = build/sdk/libaux.a
SDK = $(CRT0) $(LIBAUX) sdk/lib/aux.ld sdk/lib/libc1_s.ld sdk/tools/elf2aout.py
TESTS = build/test/hello build/test/sdk
PTYRUN = build/test/ptyrun

all: sdk $(TESTS) $(PTYRUN)

sdk: $(SDK)

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

build/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIBAUX): $(LIBAUX_SRC:%=build/sdk/libaux/%.o)
	rm -f $@
	$(AR) rcs $@ $^

build/test/%.elf: build/test/%.o $(SDK)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(CRT0) $< $(LIBAUX) -lgcc

build/test/%: build/test/%.elf sdk/tools/elf2aout.py
	$(PYTHON) sdk/tools/elf2aout.py $< $@

test-aux: $(TESTS)
	$(PYTHON) $(AUX_PKG)/tools/aux-run.py test/aux-test.sh $(TESTS)

port-%: sdk
	$(PYTHON) tools/build-port.py $*

test-port-%: sdk $(PTYRUN)
	$(PYTHON) tools/build-port.py --test $*

clean:
	rm -rf build

.PHONY: all sdk test-aux clean port-% test-port-%
.SECONDARY:

-include $(shell find build -name '*.d' 2>/dev/null)
