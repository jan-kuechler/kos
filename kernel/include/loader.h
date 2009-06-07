#ifndef LOADER_H
#define LOADER_H

#include <types.h>

enum exec_type
{
	ET_UNKNOWN,
	ET_ELF_EXEC,
};

void init_loader(void);

pid_t exec_file(const char *filename, const char *args, pid_t parent);

size_t load_file_to_mem(const char *filename, void **mem);
pid_t exec_mem(void *mem, const char *args, pid_t parent);

enum exec_type get_exec_type(void *mem);

pid_t exec_elf_exec(void *mem, const char *args, pid_t parent);

#endif /*LOADER_H*/
