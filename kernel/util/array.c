#include <stdint.h>

#ifdef HOSTED
#  include "../include/util/array.h"
#  define kmalloc malloc
#  define kcalloc calloc
#  define krealloc realloc
#  define kfree free
#else
#  include "util/array.h"
#  include "mm/kmalloc.h"
#endif

#define _my_max(a,b) ((a) > (b) ? (a) : (b))

#define AR_INIT_ELEMENTS 10
#define AR_RESIZE_FACT    2

struct array
{
	uint8_t *data; /* data buffer */
	size_t esize;  /* size of one element */
	size_t maxcnt; /* size of the buffer */
	size_t count;  /* num elements in the buffer */
};

array_t *array_create(size_t esize)
{
	array_t *ar = kmalloc(sizeof(*ar));

	ar->esize  = esize;
	ar->maxcnt = AR_INIT_ELEMENTS;
	ar->count  = 0;

#ifdef ARRAY_INIT_WITH_ZERO
	ar->data = kcalloc(AR_INIT_ELEMENTS, esize);
#else
	ar->data = kmalloc(esize * AR_INIT_ELEMENTS);
#endif

	return ar;
}

void array_destroy(array_t *ar)
{
	kfree(ar->data);
	kfree(ar);
}

size_t array_count(array_t *ar)
{
	return ar->count;
}

void *array_get(array_t *ar, size_t index)
{
	if (index >= ar->count)
		return NULL;

	return &ar->data[index * ar->esize];
}

static inline void resize(array_t *ar, size_t num)
{
	size_t old_max = ar->maxcnt;
	ar->data = krealloc(ar->data, num * ar->esize);
	ar->maxcnt = num;
	if (ar->count > num)
		ar->count = num;

#ifdef ARRAY_INIT_WITH_ZERO
	if (old_max < num) {
		memset(&ar->data[old_max * ar->esize], 0, (num - old_max) * ar->esize);
	}
#endif
}

void *array_set(array_t *ar, size_t index, void *ptr)
{
	if (index >= ar->maxcnt) {
		resize(ar, _my_max(ar->maxcnt * AR_RESIZE_FACT, index));
	}

	memcpy(&ar->data[index * ar->esize], ptr, ar->esize);
	ar->count = _my_max(ar->count, index+1);

	return &ar->data[index * ar->esize];
}

bool array_pop_back(array_t *ar, void *dst)
{
	if (!ar->count || !dst) return false;
	memcpy((uint8_t*)dst, (uint8_t*)array_get(ar, ar->count-1), ar->esize);
	ar->count -= 1;
}

size_t array_push_back(array_t *ar, void *ptr)
{
	size_t index = ar->count;
	array_set(ar, index, ptr);
	return index;
}

void array_resize(array_t *ar, size_t num)
{
	resize(ar, num);
	ar->count = num;
}

void array_reserve(array_t *ar, size_t num)
{
	resize(ar, ar->count + num);
}

void array_shrink(array_t *ar)
{
	resize(ar, ar->count);
}

#ifdef TEST

#include <stdio.h>
#include <string.h>

static void print_meminfo(array_t *ar)
{
	printf("%d ~ %d\n", ar->maxcnt * ar->esize, ar->count * ar->esize);
}

int main(void)
{
	{
		array_t *ar = array_create(sizeof(char));

		char data[] = "Hello Array!\n";
		char buff[] = "             ";
		int num = strlen(data);

		int i=0;
		for (; i < num; ++i) {
			array_set(ar, i, &data[i]);
		}

		for (i=0; i < num; ++i) {
			buff[i] = *((char*)array_get(ar, i));
		}

		printf(buff);
	}

	{
		array_t *ar = array_create(sizeof(char));
		char data[] = "Hello Array!\n";
		int num = strlen(data);

		int i=0;
		for (; i < num; ++i) {
			array_set(ar, i, &data[i]);
		}

		char *d = NULL;
		array_iterate(d, ar) {
			printf("%c", *d);
		}
		array_iterate(d, ar) {
			printf("%c", *d);
		}
	}

	{
		array_t *ar = array_create(sizeof(int));
		int data[] = {
			1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
		};
		int num = 11;

		int i=0;
		for (; i < num; ++i) {
			array_set(ar, i, &data[i]);
		}

		print_meminfo(ar);
		array_shrink(ar);
		print_meminfo(ar);

		int *d = NULL;
		array_iterate(d, ar) {
			printf("%d ", *d);
		}
		printf("\n");

		int e;
		array_iterate_as(int, e, ar) {
			printf("%d ", e);
		}
		printf("\n");
	}

	{
		typedef struct test {
			int dummy, bla;
			char foo;
			short bar;
		} test_t;

		array_t *ar = array_create(sizeof(test_t));

		test_t data[] = {
			{0,0,0,0},
			{1,3,3,7},
			{0,6,6,6},
			{42,42,42,42},
		};
		int num = 4;

		int i=0;
		for (; i < num; ++i) {
			array_set(ar, i, &data[i]);
		}

		print_meminfo(ar);
		array_shrink(ar);
		print_meminfo(ar);

		test_t *ptr;
		array_iterate(ptr, ar) {
			printf("T: %d %d %d %d\n", ptr->dummy, ptr->bla, ptr->foo, ptr->bar);
		}

		test_t e;
		array_iterate_as(test_t, e, ar) {
			printf("T: %d %d %d %d\n", e.dummy, e.bla, e.foo, e.bar);
		}

		i = 4;
		array_mutable_iterate_as(test_t, e, ar) {
			printf("T: %d %d %d %d\n", e.dummy, e.bla, e.foo, e.bar);
			if (i++ < 8) {
				test_t t = {
					i, i, i, i
				};
				array_set(ar, i, &t);
			}
		}

		array_resize(ar, 4);
		array_iterate_as(test_t, e, ar) {
			printf("T: %d %d %d %d\n", e.dummy, e.bla, e.foo, e.bar);
		}
	}

	{
		array_t *ar = array_create(sizeof(int));

		int tmp;
		int i=0;

		for (; i < 10; ++i) {
			array_push_back(ar, &i);
		}

		for (i=0; i < 10; ++i) {
			array_pop_back(ar, &tmp);
			printf("%d ", tmp);
		}
		printf("\n");
	}
}

#endif
