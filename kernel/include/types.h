#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define __depr __attribute__((deprecated))

typedef uint8_t byte   __depr;
typedef uint16_t word  __depr;
typedef uint32_t dword __depr;
typedef uint64_t qword __depr;

#endif /*TYPES_H*/
