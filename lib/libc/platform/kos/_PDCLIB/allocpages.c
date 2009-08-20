#ifndef _PDCLIB_CONFIG_H
#define _PDCLIB_CONFIG_H _PDCLIB_CONFIG_H
#include <_PDCLIB_config.h>
#endif

void *sbrk( intptr_t );

void * _PDCLIB_allocpages( int const n )
{
	void *result = sbrk(n * _PDCLIB_PAGESIZE);
	if (result == (void*)-1) {
		return NULL;
	}
	return result;
}

