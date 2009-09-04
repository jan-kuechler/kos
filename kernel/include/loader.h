#ifndef LOADER_H
#define LOADER_H

#include <stdint.h>
#include <stdlib.h>

enum exec_type
{
	ET_UNKNOWN,
	ET_ELF32_EXEC,
};

void init_loader(void);

uint32_t exec_file(const char *filename, const char *args, uint32_t parent);

size_t load_file_to_mem(const char *filename, void **mem);
uint32_t exec_mem(void *mem, const char *args, uint32_t parent);

enum exec_type get_exec_type(void *mem);

uint32_t elf32_exec(void *mem, const char *args, uint32_t parent);

#endif /*LOADER_H*/
