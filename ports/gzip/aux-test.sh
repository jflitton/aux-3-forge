# gzip's checks in A/UX, run by tools/build-port.py --test with pkg, the package, and
# sample.txt and sample.txt.gz (made by another gzip) in /usr/local/bin.
B=/usr/local/bin
fail=0
check() { if [ "$1" = 0 ]; then echo "ok   $2"; else echo "FAIL $2"; fail=1; fi; }

pkg install $B/gzip-*.pkg; check $? "pkg install"
pkg files gzip

gzip --version | sed -n 1p

mkdir /tmp/gz && cd /tmp/gz || exit 1
cp $B/sample.txt a.txt
touch -m 0101120090 a.txt
gzip a.txt; check $? "gzip a file"
test -f a.txt.gz && test ! -f a.txt; check $? "  a.txt replaced by a.txt.gz"
gzip -lv a.txt.gz
gzip -l a.txt.gz | grep ' 6146 ' >/dev/null; check $? "gzip -l: sizes"
gzip -t a.txt.gz; check $? "gzip -t"
gunzip a.txt.gz; check $? "gunzip"
cmp a.txt $B/sample.txt; check $? "  round trip matches"
ls -l a.txt | grep 'Jan  1  1990' >/dev/null; check $? "  modification time kept"

zcat $B/sample.txt.gz > b.txt; check $? "zcat another gzip's file"
cmp b.txt $B/sample.txt; check $? "  and it matches"

gzip -9 < $B/sample.txt | gzip -dc | cmp - $B/sample.txt; check $? "pipe, -9"
gzip -1 -c $B/pkg > g.gz && gzip -dc g.gz | cmp - $B/pkg; check $? "binary file, -1"

mkdir d d/e && cp $B/sample.txt d/x && cp $B/sample.txt d/e/y
gzip -r d; check $? "gzip -r"
test -f d/x.gz && test -f d/e/y.gz; check $? "  compressed both"
gzip -dr d && cmp d/e/y $B/sample.txt; check $? "gzip -dr"

gzip nonexistent 2>err; test $? = 1; check $? "missing file exits 1"
grep "nonexistent: No such file or directory" err >/dev/null; check $? "  and says so"
echo hello > h && gzip h && gzip h.gz 2>err; check $? "already .gz: left alone, exit 0"
grep "already has .gz suffix" err >/dev/null; check $? "  and says so"
echo "x" | gzip -dc 2>err; test $? = 1; check $? "not gzip data exits 1"
grep "not in gzip format" err >/dev/null; check $? "  and says so"

pkg remove gzip; check $? "pkg remove"
# (A/UX's sh has no "!", and its test no -h)
if ls $B/gzip $B/gunzip $B/zcat >/dev/null 2>&1; then false; fi; check $? "  removed its files"

exit $fail
