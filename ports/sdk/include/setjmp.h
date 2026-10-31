/*
 * libaux's setjmp and longjmp. A jmp_buf holds d2-d7, a2-a6, the stack pointer, the
 * return address and fp2-fp7, which gcc keeps across calls when it uses the FPU (A/UX's
 * own, for its cc, has no room for them). setjmp doesn't save the signal mask;
 * sigsetjmp does if asked, after the jmp_buf.
 */
#ifndef _SETJMP_H
#define _SETJMP_H

typedef long jmp_buf[31];
typedef long sigjmp_buf[33];

int setjmp(jmp_buf env);
int _setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val) __attribute__((noreturn));
void _longjmp(jmp_buf env, int val) __attribute__((noreturn));
int sigsetjmp(sigjmp_buf env, int savemask);
void siglongjmp(sigjmp_buf env, int val) __attribute__((noreturn));

#endif
