#ifndef UTIL_ARRAY_H
#define UTIL_ARRAY_H

#include <stdbool.h>
#include <stdlib.h>

struct array;
typedef struct array array_t;

/**
 * Define ARRAY_INIT_WITH_ZERO to let uninitialized
 * array data filled with 0s.
 */
#define ARRAY_INIT_WITH_ZERO

/**
 *  array_create(esize)
 *
 * Creates an array for elements of esize bytes.
 */
array_t *array_create(size_t esize);

/**
 *  array_destroy(ar)
 *
 * Destroys an array.
 */
void array_destroy(array_t *ar);

/**
 *  array_count(ar)
 *
 * Returns the number of elements in the array.
 */
size_t array_count(array_t *ar);

/**
 *  array_get(ar, index)
 *
 * Returns a pointer to the element at the given index,
 * or NULL, if the index is invalid (index >= array_count(ar))
 */
void *array_get(array_t *ar, size_t index);

/**
 *  array_get_as(type, ar, index)
 *
 * Returns the element at the given index, casted to 'type'.
 * Only valid for i < array_count(ar) and types that match the
 * element size of the array.
 */
#define array_get_as(t,a,i) (*(t*)(array_get((a),(i))))

/**
 *  array_set(ar, index, value)
 *
 * Sets the element at the given index to the value
 * ptr points to and returns a pointer to the element in the array.
 * If the array is not large enough it gets extended automatically.
 */
void *array_set(array_t *ar, size_t index, void *ptr);

/**
 *  array_push_back(ar, ptr)
 *
 * Adds the value at the end of the array and returns it's index.
 */
size_t array_push_back(array_t *ar, void *ptr);

/**
 *  array_pop_back(array_t *ar, void *dst);
 *
 * Copies the last element of the array to dst and removes it from
 * the array. It returns true on success.
 */
bool array_pop_back(array_t *ar, void *dst);

/**
 *  array_resize(ar, num)
 *
 * Resizes the array to be exactly num elements wide.
 */
void array_resize(array_t *ar, size_t num);

/**
 *  array_reserve(ar, num)
 *
 * Reserves size for additional num elements. This speeds
 * up the execution speed of many array_sets in a row.
 */
void array_reserve(array_t *ar, size_t num);

/**
 *  array_shrink(ar)
 *
 * Shrinks the array to optimal size. Use this if you're sure
 * no new data gets added for some time and you want to free
 * unused memory.
 */
void array_shrink(array_t *ar);

/** Warning: Ugly macro magic and for-loops ahead! **/
/** Note:
 *  Due to macro limitations you must not use more than one array_*iterate*
 *  loop per line of source code.
 *  The following example is _not_ allowed:
 *   array_iterate(a, array1) { array_iterate(b, array2) { ... } }
 *  Just use:
 *   array_iterate(a, array1) {
 *     array_iterate(b, array2) { ... }
 *   }
 */

#define _ar_cat_(a,b) a##b
#define _ar_cat(a,b) _ar_cat_(a,b)

/**
 *  array_iterate(ptr, ar)
 *
 * Iterates over the array and assigns each array_get return value to
 * ptr. This can be used like a for/while loop.
 */
#define array_iterate(ptr,ar) \
	int _ar_cat(_ar_i,__LINE__) = 0; \
	while (ptr = array_get((ar), (_ar_cat(_ar_i,__LINE__))++))

/* => int i=0; while (ptr = array_get(ar), i++) */

/**
 *  array_iterate_as(t, el, ar)
 *
 * Iterates over the array and assigns each array element to el, which is
 * of the type t.
 * You *must not* call array_set inside the loop's body. Calling array_set
 * or array_resize invalidates the internal iterator and leads to
 * undefined results (e.g. NULL ptr dereference).
 * If you need to add values inside the loop, use array_mutable_iterate_as.
 */
#define array_iterate_as(t,e,ar) \
	int _ar_cat(_ar_i,__LINE__) = 0; \
	int _ar_cat(_ar_c,__LINE__) = array_count(ar); \
	for (; (_ar_cat(_ar_i,__LINE__)) < _ar_cat(_ar_c,__LINE__) && \
	     (e = array_get_as(t,(ar),_ar_cat(_ar_i,__LINE__)++), 1); )

/* => int i=0, count = array_count(ar); for (; i < count && (e = array_get_as(type, ar, i++), 1);) */

/**
 *  array_mutable_iterate_as(t, el, ar)
 *
 * Iterates over the array and assigns each array element to el, which is
 * of the type t. This loop is slower than array_iterate_as, but is not
 * limited in the use of array_set.
 */
#define array_mutable_iterate_as(t,e,ar) \
	int _ar_cat(_ar_i,__LINE__) = 0; \
	for (; (_ar_cat(_ar_i,__LINE__)) < array_count(ar) && \
	     (e = array_get_as(t,(ar),_ar_cat(_ar_i,__LINE__)++), 1); )

/* => int i=0;for (; i < array_count(ar) && (e = array_get_as(type, ar, i++), 1);) */

#endif /*UTIL_ARRAY_H*/
