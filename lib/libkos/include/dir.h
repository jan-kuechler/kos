#ifndef LIBKOS_DIR_H
#define LIBKOS_DIR_H

#include <stdint.h>

struct libkos_DIR
{
	char     **names;
	uint32_t index;
	uint32_t count;

	struct dirent dirent;
};

#endif /*LIBKOS_DIR_H*/
