/*
 * What gcc's -fstack-protector needs: the guard it puts below a function's arrays, and
 * what a function calls when it finds the guard changed on return. The guard holds a
 * zero byte, a newline and 0xff, which string functions stop at.
 */
#include <stdlib.h>
#include <unistd.h>

unsigned long __stack_chk_guard = 0x000aff0dUL;

void
__stack_chk_fail(void)
{
	static const char msg[] = "stack smashing detected\n";

	write(2, msg, sizeof msg - 1);
	abort();
}
