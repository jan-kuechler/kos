#ifndef PARAM_H
#define PARAM_H

#include <stdbool.h>
#include "linker.h"

struct parameter
{
	const char *name;
	int (*func)(char*);
};

#define MAKE_BOOT_PARAM(name,id,func) \
	static char __boot_param_str_##id[] __initdata = name; \
	static struct parameter __boot_param_##id \
	__initparam __attribute((used, aligned(1))) \
	= { __boot_param_str_##id, func }

#define BOOT_PARAM(name,func) MAKE_BOOT_PARAM(name,func,func)

#define BOOT_FLAG(name,flag,value) \
	static int do_##name(char *val) { \
		flag = value; \
		return 0; \
	} \
	BOOT_PARAM(#name,do_##name)

linker_symbol(init_param_start);
linker_symbol(init_param_end);

#define init_param_size (init_param_end - init_param_start)
#define num_init_params (init_param_size / sizeof(struct parameter))

int parse_params(char *cmdline, struct parameter *params, unsigned int num, bool noerror);
void print_params(struct parameter *params, unsigned int num);

#endif /*PARAM_H*/
