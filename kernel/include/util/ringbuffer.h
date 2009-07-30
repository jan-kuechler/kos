#ifndef UTIL_RINGBUFFER_H
#define UTIL_RINGBUFFER_H

#include <stdbool.h> /* bool   */
#include <stdlib.h>  /* size_t */

struct ringbuffer;
typedef struct ringbuffer ringbuffer_t;

/**
 *  rbuf_create(esize, num, override)
 *
 * Creates and returns a ringbuffer large enough to hold
 * num items of esize bytes each.
 * If override is true old data may be overriden if there
 * is not enough space to complete a write request.
 * You have to call rbuf_destroy to free the ringbuffer.
 */
ringbuffer_t *rbuf_create(size_t esize, size_t num, bool override);

/**
 *  rbuf_init_static(rb, buf, esize, num, override)
 *
 * Initializes a static ringbuffer with the given parameters
 * and the pointer to a buffer of at least esize*num bytes
 * length.
 */
void rbuf_init_static(ringbuffer_t *rb, void *buf, size_t esize,
                      size_t num, bool override);

/**
 *  rbuf_destroy(rb)
 *
 * Destroys a ringbuffer and frees its memory.
 * It is not allowed to call rbuf_destroy on a ringbuffer initialized
 * with rbuf_init_static!
 */
void rbuf_destroy(ringbuffer_t *rb);

/**
 *  rbuf_empty(rb)
 *
 * Returns true if the ringbuffer is empty,
 * false otherwise.
 */
bool rbuf_empty(ringbuffer_t *rb);

/**
 *  rbuf_avail(rb)
 *
 * Returns the number of items in the buffer.
 */
size_t rbuf_avail(ringbuffer_t *rb);

/**
 *  rbuf_freesize(rb)
 *
 * Returns the number of items that still can go into the buffer.
 */
size_t rbuf_freesize(ringbuffer_t *rb);

/**
 *  rbuf_write(rb, src, count)
 *
 * Writes count items from src into the buffer. It returns the number
 * of items written, which may be lower than count if the override flag
 * is not set and there is not enough space to write every item.
 * If rbuf_write returns a number lower than count, the buffer is full.
 */
size_t rbuf_write(ringbuffer_t *rb, void *src, size_t count);

/**
 *  rbuf_read(rb, dst, count)
 *
 * Reads count items from the buffer to the destination. Dst must point
 * to at least count * element-size bytes!
 * The number items read is returned. If the return value is lower than
 * count the ringbuffer is empty.
 */
size_t rbuf_read(ringbuffer_t *rb, void *dst, size_t count);

#endif /*UTIL_RINGBUFFER_H*/
