#ifndef _TYPES_H_
#define _TYPES_H_
#include <stddef.h>

typedef signed int ssize_t;

typedef void * paddr_t;
typedef void * vaddr_t;

typedef unsigned int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;

#endif

// FIXME Um pid_t aus sys/types holen zu koennen
#ifndef _NO_LOST_TYPES

// Schutz vor mehrfachen Deklarationen
#ifndef _LOST_TYPES_
#define _LOST_TYPES_

#ifndef __cplusplus
// FIXME: HACK
#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

typedef enum { FALSE = 0, TRUE } bool;
#endif


typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long qword;

typedef signed char sbyte;
typedef signed short sword;
typedef signed int sdword;
typedef signed long long sqword;

typedef qword timestamp_t;

typedef dword syscall_id_t;
#endif // ndef _LOST_TYPES_
#endif // ndef _NO_LOST_TYPES

