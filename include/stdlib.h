#ifndef _STDLIB_H
#define _STDLIB_H

#include "mm/kmalloc.h"

#define malloc  kmalloc
#define free    kfree
#define realloc krealloc
#define callc   kcalloc

#endif
