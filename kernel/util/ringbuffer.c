#include <stdint.h>

#ifdef HOSTED
#  include "../include/util/ringbuffer.h"
#  define kmalloc malloc
#  define kfree   free
#else
#  include "util/ringbuffer.h"
#  include "mm/kmalloc.h"
#endif

struct ringbuffer
{
	uint8_t *data;     /* ptr to mem for the buffer */

	size_t esize;      /* size of an element */
	size_t maxcnt;     /* max number of elements */
	size_t count;      /* cur number of elements */

	size_t ridx, widx; /* read-/write-index */

	bool override;     /* override if full? */
};

ringbuffer_t *rbuf_create(size_t esize, size_t num, bool override)
{
	ringbuffer_t *rb = kmalloc(sizeof(*rb));
	if (!rb)
		return NULL;

	rb->esize = esize;
	rb->maxcnt = num;
	rb->override = override;

	rb->data  = kmalloc(esize * num);
	if (!rb->data) {
		kfree(rb);
		return NULL;
	}

	rb->count = 0;

	rb->ridx = 0;
	rb->widx = 0;

	return rb;
}

void rbuf_init_static(ringbuffer_t *rb, void *data, size_t esize,
                      size_t num, bool override)
{
	if (!rb || !data) return;

	rb->data     = data;
	rb->esize    = esize;
	rb->maxcnt   = num;
	rb->override = override;
	rb->count    = 0;
	rb->ridx     = 0;
	rb->widx     = 0;
}

void rbuf_destroy(ringbuffer_t *rb)
{
	kfree(rb->data);
	kfree(rb);
}

bool rbuf_empty(ringbuffer_t *rb)
{
	return rb->count == 0;
}

size_t rbuf_avail(ringbuffer_t *rb)
{
	return rb->count;
}

size_t rbuf_freesize(ringbuffer_t *rb)
{
	return rb->maxcnt - rb->count;
}

size_t rbuf_write(ringbuffer_t *rb, void *src, size_t count)
{
	size_t num = count;
	if (count > rbuf_freesize(rb) && !rb->override) {
		num = rbuf_freesize(rb);
	}

	uint8_t *bsrc = src;
	int i=0;
	for (; i < num; ++i) {
		memcpy(&rb->data[rb->widx], &bsrc[i*rb->esize], rb->esize);
		rb->widx += rb->esize;
		rb->widx %= (rb->maxcnt * rb->esize);
	}

	rb->count += num;

	return num;
}

size_t rbuf_read(ringbuffer_t *rb, void *dst, size_t count)
{
	size_t num = count;
	if (count > rbuf_avail(rb)) {
		num = rbuf_avail(rb);
	}

	uint8_t *bdst = dst;
	int i=0;
	for (; i < num; ++i) {
		memcpy(&bdst[i * rb->esize], &rb->data[rb->ridx], rb->esize);
		rb->ridx += rb->esize;
		rb->ridx %= (rb->maxcnt * rb->esize);
	}

	rb->count -= num;

	return num;
}

#if defined(TEST) && defined(PERF)
#  error "Cannot create test and performace test at the same time..."
#endif

#ifdef TEST

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	{
		ringbuffer_t *rb = rbuf_create(1, 20, false);
		assert(rb);
		assert(rb->data);
		assert(rbuf_empty(rb));
		assert(rbuf_avail(rb) == 0);
		assert(rbuf_freesize(rb) == 20);

		char src[] = "Hallo!";
		size_t len = 7; /* strlen(src) + 1 */

		size_t num = rbuf_write(rb, src, len);
		assert(num == len);
		assert(!rbuf_empty(rb));
		assert(rbuf_avail(rb) == len);
		assert(rbuf_freesize(rb) == (20 - len));

		char dst[7] = {0};

		num = rbuf_read(rb, dst, len);
		assert(num == len);
		assert(strcmp(src, dst) == 0);
		assert(rbuf_empty(rb));
		assert(rbuf_avail(rb) == 0);
		assert(rbuf_freesize(rb) == 20);

		char large[]         = "This is a very large string...";
		char large_trimmed[] = "This is a very large"; // len == 20
		size_t llen = 31;

		num = rbuf_write(rb, large, llen);
		assert(num == 20);

		char ldst[31] = {0};
		num = rbuf_read(rb, ldst, llen);
		assert(num == 20);
		assert(strcmp(large_trimmed, ldst) == 0);
	}
	{
		ringbuffer_t *rb = rbuf_create(sizeof(int), 20, false);
		assert(rb);
		assert(rb->data);
		assert(rbuf_empty(rb));
		assert(rbuf_avail(rb) == 0);
		assert(rbuf_freesize(rb) == 20);

		int src[] = {1, 2, 3, 4, 5};
		size_t len = 5;

		size_t num = rbuf_write(rb, src, len);
		assert(num == len);
		assert(num == len);
		assert(!rbuf_empty(rb));
		assert(rbuf_avail(rb) == len);
		assert(rbuf_freesize(rb) == (20 - len));

		int dst[5] = {0};
		num = rbuf_read(rb, dst, len);
		assert(num == len);
		assert(memcmp(src, dst, len) == 0);
		assert(rbuf_empty(rb));
		assert(rbuf_avail(rb) == 0);
		assert(rbuf_freesize(rb) == 20);
	}

	{
		ringbuffer_t rb;
		float data[20];
		rbuf_init_static(&rb, data, sizeof(float), 20, false);
	}

	printf("All tests passed!\n\n");

	return 0;
}

#endif

#ifdef PERF

#include <assert.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* GetTickCount */

#define PNUM 10000

#define PSTART(Name) \
	{ \
		const char *_name = Name; \
		DWORD _start_tick = GetTickCount(); \
		int _count = 0; \
		for (; _count < PNUM; ++_count) {

#define PEND \
		} \
		DWORD _end_tick = GetTickCount(); \
		DWORD _total = _end_tick - _start_tick; \
		printf("%s took %d ms\n", _name, _total); \
	}


int main(int argc, char **argv)
{
	ringbuffer_t *buffers[PNUM] = {0};

	int full_data[100];
	int full_dest[100] = {0};
	memset(full_data, 0x42, 100 * sizeof(int));

	int part_data[15];
	int part_dest[15] = {0};
	memset(part_data, 0x42, 15 * sizeof(int));

	printf("Performance tests:\n");

	PSTART("Empty")
	PEND

	PSTART("Create")
		buffers[_count] = rbuf_create(sizeof(int), 100, false);
	PEND

	PSTART("Full-Write")
		rbuf_write(buffers[_count], full_data, 100);
	PEND

	PSTART("Full-Read")
		rbuf_read(buffers[_count], full_dest, 100);
	PEND

	assert(memcmp(full_data, full_dest, 100 * sizeof(int)) == 0);

	PSTART("Write-Read-Cycles")
		int i=0;
		for (; i < 10; ++i) {
			rbuf_write(buffers[_count], part_data, 15);
			rbuf_read(buffers[_count], part_dest, 15);
		}
	PEND

	assert(memcmp(part_data, part_dest, 15 * sizeof(int)) == 0);

	PSTART("Destroy")
		rbuf_destroy(buffers[_count]);
	PEND

	printf("End.\n");
}

#endif
