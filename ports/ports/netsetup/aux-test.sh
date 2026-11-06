# netsetup's checks in A/UX, run by tools/build-port.py --test with pkg and the package in
# /usr/local/bin. QEMU gives A/UX no Ethernet it can drive, so DHCP itself isn't tried
# here: these check the package, the files netsetup keeps the settings in, and lo0.
B=/usr/local/bin
fail=0
check() { if [ "$1" = 0 ]; then echo "ok   $2"; else echo "FAIL $2"; fail=1; fi; }
has() { grep "$1" "$2" >/dev/null; check $? "$3"; }
hasnt() { if grep "$1" "$2" >/dev/null; then check 1 "$3"; else check 0 "$3"; fi; }
out=/tmp/netsetup.out

pkg install $B/netsetup-*.pkg > $out 2>&1; check $? "pkg install"
has 'Added /usr/local/bin/netsetup boot to /etc/inittab' $out "  says it added the boot line"
has '^pk00:2:wait:/usr/local/bin/netsetup boot </dev/syscon' /etc/inittab \
	"  the boot line is in /etc/inittab"
test -f /mac/lib/cmdo/n/netsetup; check $? "  the Commando dialog is in /mac/lib/cmdo"

netsetup > $out 2>&1; check $? "netsetup alone"
has '^gateway' $out "  shows the gateway"
has '^DNS servers' $out "  and the DNS servers"
netsetup lo0 > $out 2>&1; check $? "netsetup lo0"
has '^lo0 .*127\.0\.0\.1, netmask 255\.0\.0\.0' $out "  shows its address"
netsetup nosuch0 > $out 2>&1
has "there's no interface nosuch0" $out "an interface that isn't there"
netsetup lo0 dhcp gateway 1.2.3.4 > $out 2>&1
has 'dhcp goes alone' $out "dhcp with an address is refused"
dhcpc > $out 2>&1
has '^usage: dhcpc' $out "dhcpc's usage"

echo 'domain example.com' > /etc/resolv.conf
echo 'nameserver 10.9.9.9' >> /etc/resolv.conf
netsetup dns 192.168.1.1,8.8.8.8 > $out 2>&1; check $? "netsetup dns"
has '^nameserver 192.168.1.1$' /etc/resolv.conf "  the first server"
has '^nameserver 8.8.8.8$' /etc/resolv.conf "  the second"
hasnt '10.9.9.9' /etc/resolv.conf "  not the old one"
has '^domain example.com$' /etc/resolv.conf "  keeping the domain"
netsetup dns none; hasnt nameserver /etc/resolv.conf "netsetup dns none"

# by hand on lo0, with its own address, so loopback keeps working
netsetup lo0 127.0.0.1 netmask 255.0.0.0 gateway none dns 127.0.0.1 > $out 2>&1
check $? "an address by hand"
has '^lo0: 127.0.0.1, netmask 255.0.0.0, gateway none' $out "  says what it set"
has '^lo0	127.0.0.1 127.255.255.255 255.0.0.0$' /etc/NETADDRS "  NETADDRS has its line"
has '^nameserver 127.0.0.1$' /etc/resolv.conf "  and resolv.conf its DNS server"
/usr/etc/ping 127.0.0.1 64 1 > $out 2>&1; check $? "  loopback still works"
netsetup lo0 > $out 2>&1
has '^lo0 *by hand' $out "  netsetup lo0 says it's by hand"

# DHCP on lo0 fails (it has no Ethernet address), but the settings change
netsetup lo0 dhcp > $out 2>&1
has '^lo0: by DHCP from now on' $out "dhcp"
has "can't read lo0's Ethernet address" $out "  dhcpc says why it can't on lo0"
has '^lo0$' /etc/dhcpc.conf "  dhcpc.conf lists it"
hasnt '^lo0' /etc/NETADDRS "  and NETADDRS doesn't"
netsetup boot > $out 2>&1; check $? "netsetup boot"
netsetup lo0 off > $out 2>&1; check $? "off"
hasnt '^lo0$' /etc/dhcpc.conf "  dhcpc.conf doesn't list it"
netsetup lo0 > $out 2>&1
has '(down)' $out "  it's down"
/etc/ifconfig lo0 up

pkg verify netsetup > $out 2>&1; check $? "pkg verify"
pkg remove netsetup > $out 2>&1; check $? "pkg remove"
has 'Took /usr/local/bin/netsetup boot out of /etc/inittab' $out "  takes the boot line out"
hasnt netsetup /etc/inittab "  it's gone from /etc/inittab"
if ls $B/netsetup $B/dhcpc /mac/lib/cmdo/n/netsetup >/dev/null 2>&1; then false; fi
check $? "  and its files"
test -d /mac/lib/cmdo/n; check $? "  but not A/UX's /mac/lib/cmdo/n"

exit $fail
