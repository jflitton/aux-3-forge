# Build gzip with the SDK and install it into $STAGE. GNU_STANDARD=0: gzip acts on the
# name it's run by (gunzip, zcat), as gzip.c offers for systems with links.
"$SRC/configure" --host=m68k-apple-aux3.1 --prefix=/usr/local --mandir=/usr/local/man \
	CFLAGS='-Os -DGNU_STANDARD=0'
make -j4
make install DESTDIR="$STAGE"

# gzip's other commands are shell scripts written for a POSIX shell, which A/UX's
# /bin/sh predates. gunzip and zcat are scripts too, so they become links to gzip.
# uncompress (a link to the gunzip script) would hide A/UX's own.
cd "$STAGE/usr/local"
rm -rf share
for f in gzexe uncompress zcmp zdiff zegrep zfgrep zforce zgrep zless zmore znew; do
	rm -f bin/$f man/man1/$f.1
done
rm -f bin/gunzip bin/zcat
ln -s gzip bin/gunzip
ln -s gzip bin/zcat

# For the test: a file, and a copy compressed by the gzip on this machine.
cp "$SRC/README" "$TESTDIR/sample.txt"
gzip -9 -n -c "$SRC/README" > "$TESTDIR/sample.txt.gz"
