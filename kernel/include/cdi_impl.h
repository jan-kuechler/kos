#ifndef CDI_IMPL_H
#define CDI_IMPL_H

#include <kos/config.h>
#include "kernel.h"

struct list; /* from util/list.h */
/* Define cdi_list_implementation here to let all functions that use
   cdi_lists have access to helpers like list_iterate */
struct cdi_list_implementation
{
	struct list *list;
};

#define UNIMPLEMENTED panic("The CDI function %s is not implemented yet.", __func__);

#ifdef CONF_CDI_ERR_FATAL
#  define cdi_error(msg, ...) \
		do { \
			panic("Error in CDI function %s: %s.", __func__, msg); \
			return __VA_ARGS__; \
		} while (0)
#else
#  include "debug.h"
#  define cdi_error(msg, ...) \
		do { \
			dbg_error("Error in CDI function %s: %s.", __func__, msg); \
			return __VA_ARGS__; \
		} while (0)
#endif

#ifdef CONF_CDI_SECURE
#  define cdi_check_init(...)  \
		do { \
			extern int cdi_initialized; \
			if (!cdi_initialized) { \
				cdi_error("CDI is not initialized", __VA_ARGS__); \
			} \
		} while (0)
#  define cdi_check_arg(arg, check, ...) \
	  if (!(arg check)) { \
			cdi_error("Argument " #arg " is invalid", __VA_ARGS__); \
		}
#else
#  define cdi_check_init(...)
#  define cdi_check_arg(arg, check, ...) \
		if (!(arg check)) { \
			return __VA_ARGS__; \
		}
#endif

#endif /*CDI_IMPL_H*/
