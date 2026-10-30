/*
 * Heap, as aux-pkg's rt/libc.c: first fit from a free list kept in address order, so
 * that freeing can merge a block with its neighbours. Each block starts with its size,
 * header included, and is a multiple of 8 bytes. The heap grows with the shared
 * library's sbrk. malloc and realloc themselves are in malloc-a0.S.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "libaux.h"

struct block {
	size_t size;
	struct block *next;	/* while free */
};

#define HDR		8
#define GROW		32768
static struct block *freelist;

static void
release(struct block *b)
{
	struct block **pp = &freelist, *p;

	while ((p = *pp) && p < b)
		pp = &p->next;
	b->next = p;
	if (p && (char *)b + b->size == (char *)p) {
		b->size += p->size;
		b->next = p->next;
	}
	*pp = b;
	if (pp != &freelist) {
		struct block *prev = (struct block *)((char *)pp -
			offsetof(struct block, next));

		if ((char *)prev + prev->size == (char *)b) {
			prev->size += b->size;
			prev->next = b->next;
		}
	}
}

void *
__aux_malloc(size_t n)
{
	struct block **pp, *b;
	size_t need = (n + HDR + 7) & ~(size_t)7;

	if (n > 0x7fff0000) {
		errno = ENOMEM;
		return NULL;
	}
	for (;;) {
		for (pp = &freelist; (b = *pp); pp = &b->next) {
			if (b->size < need)
				continue;
			if (b->size - need >= 16) {
				struct block *rest = (struct block *)((char *)b + need);

				rest->size = b->size - need;
				rest->next = b->next;
				*pp = rest;
				b->size = need;
			} else
				*pp = b->next;
			return (char *)b + HDR;
		}
		/* keep the break 8-aligned, then grow by at least GROW */
		{
			size_t pad = -(size_t)(unsigned long)sbrk(0) & 7;
			size_t grow = need > GROW ? need : GROW;

			if (pad && sbrk(pad) == (void *)-1)
				return NULL;
			b = sbrk(grow);
			if (b == (void *)-1) {
				errno = ENOMEM;
				return NULL;
			}
			b->size = grow;
			release(b);
		}
	}
}

void
free(void *p)
{
	if (p)
		release((struct block *)((char *)p - HDR));
}

void *
__aux_realloc(void *p, size_t n)
{
	size_t have;
	void *q;

	if (!p)
		return __aux_malloc(n);
	have = ((struct block *)((char *)p - HDR))->size - HDR;
	if (n <= have)
		return p;
	if (!(q = __aux_malloc(n)))
		return NULL;
	memcpy(q, p, have);
	free(p);
	return q;
}
