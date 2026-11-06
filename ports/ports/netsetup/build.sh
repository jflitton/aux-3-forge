# Build netsetup and dhcpc, from this port's own source ($SRC, ports/netsetup/src), and
# install them and netsetup's Commando dialog into $STAGE.
for p in netsetup dhcpc; do
	m68k-apple-aux3.1-gcc -Os -Wall -Wextra -Wno-unused-parameter -o $p \
		"$SRC/$p.c" "$SRC/net.c" "$SRC/dhcp.c"
done
mkdir -p "$STAGE/usr/local/bin" "$STAGE/mac/lib/cmdo/n"
cp netsetup dhcpc "$STAGE/usr/local/bin/"
chmod 755 "$STAGE/usr/local/bin/netsetup" "$STAGE/usr/local/bin/dhcpc"
cp "$SRC/netsetup.cmdo" "$STAGE/mac/lib/cmdo/n/netsetup"
chmod 644 "$STAGE/mac/lib/cmdo/n/netsetup"
