# mksh's checks in A/UX, run by tools/build-port.py --test with pkg, the package and ptyrun
# in /usr/local/bin. ptyrun types at an interactive mksh on a pseudo-terminal; each check
# looks for output only that keystroke's working gives.
B=/usr/local/bin
fail=0
check() { if [ "$1" = 0 ]; then echo "ok   $2"; else echo "FAIL $2"; fail=1; fi; }
same() { if [ "$1" = "$2" ]; then check 0 "$3"; else check 1 "$3: got '$1'"; fi; }

pkg install $B/mksh-*.pkg; check $? "pkg install"
pkg files mksh
grep '^/usr/local/bin/mksh$' /etc/shells >/dev/null; check $? "  added to /etc/shells"
M=$B/mksh
HOME=/tmp/mkshtest ENV=/dev/null
export HOME ENV
mkdir $HOME
echo completion-worked > $HOME/completeme

$M -c 'echo $KSH_VERSION' | grep 'MIRBSD KSH R59' >/dev/null; check $? "runs: \$KSH_VERSION"
echo 'typeset -i n=6; set -A a x y z' >$HOME/s1
echo 'f() { typeset l=$1; echo $((n * 7)) ${a[1]} ${#a[*]} $l; }; f arg' >>$HOME/s1
same "`$M $HOME/s1`" "42 y 3 arg" "arithmetic, arrays, functions"
same "`$M -c 'x=hello.tar.gz; echo ${x%%.*} ${x#*.} ${x/l/L}'`" \
	"hello tar.gz heLlo.tar.gz" "parameter expansion"
same "`$M -c '[[ abc == a* && -d / ]] && echo yes'`" "yes" "[[ ]]"
same "`$M -c 'echo $(echo in $(echo nested))'`" "in nested" "command substitution"
same "`$M -c 'echo ~root'`" "/" "~root (getpwnam)"
$M -c 'x=$((1/0)); echo not reached' >$HOME/out 2>&1
same "$?:`grep -c 'not reached' $HOME/out`" "1:0" "an error stops a script (longjmp)"
$M -c 'trap "echo trapped" USR1; kill -USR1 $$; echo after' >$HOME/out
same "`tr '\012' ' ' <$HOME/out`" "trapped after " "trap"
same "`$M -c '/bin/sleep 40 & /bin/sleep 1; kill $!; wait $!; echo $?'`" "143" \
	"a killed job's status"
same "`$M -c 'kill -l 20'`" "TSTP" "kill -l"
echo 'echo $# $2' > $HOME/script
same "`$M $HOME/script a b`" "2 b" "a script with arguments"

# interactive, on a pseudo-terminal; A/UX's terminals have no suspend key until set
ptyrun 'stty susp \^Z\r' 'echo hist-$((6*7))\r' '\e[A\r' 'cho edited-$((1+1))^Ae\r' \
	"cat $HOME/compl\\t\\r" '/bin/sleep 30\r' '^Z' 'kill %1\r' 'exit\r' \
	-- $M -i >$HOME/tty1 2>&1
HISTFILE=$HOME/hist
export HISTFILE
ptyrun 'echo saved-$((5*5))\r' 'exit\r' -- $M -i >$HOME/tty2 2>&1
ptyrun '\e[A\e[A\r' 'exit\r' -- $M -i >$HOME/tty3 2>&1	# (the last is exit)
same "`grep -c hist-42 $HOME/tty1`" 2 "up arrow brings back the last command"
grep edited-2 $HOME/tty1 >/dev/null; check $? "control-A goes to the start of the line"
grep completion-worked $HOME/tty1 >/dev/null; check $? "tab completes a file name"
grep 'Stopped.*/bin/sleep' $HOME/tty1 >/dev/null; check $? "control-Z stops a job"
grep 'Terminated.*/bin/sleep' $HOME/tty1 >/dev/null; check $? "  kill %1 ends it"
grep saved-25 $HOME/tty3 >/dev/null; check $? "history is kept in \$HISTFILE"
if [ $fail != 0 ]; then
	echo "--- first session:"; cat $HOME/tty1
fi
rm -rf $HOME

pkg remove mksh; check $? "pkg remove"
if ls $M >/dev/null 2>&1; then false; fi; check $? "  removed its files"
if grep mksh /etc/shells >/dev/null; then false; fi; check $? "  and its /etc/shells line"

exit $fail
