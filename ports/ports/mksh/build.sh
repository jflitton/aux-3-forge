# Build mksh with the SDK and install it into $STAGE. mksh's Build.sh knows A/UX and only
# compiles and links its test programs, so it works when cross-compiling. LIBS is empty:
# libaux has what it would take from A/UX's /lib/libposix.a. -r: no preformatted manual.
TARGET_OS=A/UX TARGET_OSREV=3.1 CC=m68k-apple-aux3.1-gcc CFLAGS=-Os LIBS= \
	sh "$SRC/Build.sh" -r
mkdir -p "$STAGE/usr/local/bin" "$STAGE/usr/local/man/man1"
cp mksh "$STAGE/usr/local/bin/mksh"
chmod 755 "$STAGE/usr/local/bin/mksh"
# the manual also carries mksh's licence, which asks to go with the program
cp "$SRC/mksh.1" "$STAGE/usr/local/man/man1/mksh.1"
chmod 644 "$STAGE/usr/local/man/man1/mksh.1"
