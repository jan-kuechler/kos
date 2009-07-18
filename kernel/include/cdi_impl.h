#ifndef CDI_IMPL_H
#define CDI_IMPL_H

#include "kernel.h"

#define UNIMPLEMENTED panic("The CDI function %s is not implemented yet.", __func__);

#define cdi_error(msg, ...) do { panic("Error in CDI function %s: %s.", __func__, msg); __VA_ARGS__; } while (0)

#define cdi_check_init(...)  \
	do { \
		extern int cdi_initialized; \
		if (!cdi_initialized) { \
			cdi_error("CDI is not initialized"); \
			__VA_ARGS__; \
		} \
	} while (0)


#define cdi_check_arg(arg, check) \
  if (!(arg check)) { \
		cdi_error("Argument " #arg " is invalid"); \
	}

#endif /*CDI_IMPL_H*/
