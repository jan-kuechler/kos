#ifndef UTIL_RINGBUFFER_H
#define UTIL_RINGBUFFER_H

#ifdef RBUF_TEST
# include <stdlib.h>
# define kmalloc malloc
# define kfree   free
#else
# include "mm/kmalloc.h"
#endif

typedef struct rinbuffer
{
	char *buffer;
	char *head, *tail;
	unsigned int length;
	unsigned int esize;
} rinbuffer_t;

static inline void rbuf_init(rinbuffer_t *buf, unsigned int esize, unsigned int len)
{
	buf->esize = esize;
	buf->length = len;
	buf->buffer = kmalloc(len * esize);
	buf->head   = buf->buffer;
	buf->tail   = buf->buffer;
}

static inline void rbuf_put(ringbuffer_t *buf, void *data, unsigned int num)
{

}

#endif /*UTIL_RINGBUFFER_H*/
