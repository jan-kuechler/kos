#include <ctype.h>

#ifndef _CONST
#define _CONST const
#endif

#define _CTYPE_DATA_0_127 \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C, \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_C,	_C,	_C,	_C,	_C,	_C,	_C,	_C, \
	_S|_B,	_P,	_P,	_P,	_P,	_P,	_P,	_P, \
	_P,	_P,	_P,	_P,	_P,	_P,	_P,	_P, \
	_N,	_N,	_N,	_N,	_N,	_N,	_N,	_N, \
	_N,	_N,	_P,	_P,	_P,	_P,	_P,	_P, \
	_P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U, \
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U, \
	_U,	_U,	_U,	_U,	_U,	_U,	_U,	_U, \
	_U,	_U,	_U,	_P,	_P,	_P,	_P,	_P, \
	_P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L, \
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L, \
	_L,	_L,	_L,	_L,	_L,	_L,	_L,	_L, \
	_L,	_L,	_L,	_P,	_P,	_P,	_P,	_C

#define _CTYPE_DATA_128_256 \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0, \
	0,	0,	0,	0,	0,	0,	0,	0

#if defined(__GNUC__) && !defined(__CHAR_UNSIGNED__) && !defined(COMPACT_CTYPE)
#define ALLOW_NEGATIVE_CTYPE_INDEX
#endif

#if defined(ALLOW_NEGATIVE_CTYPE_INDEX)
static _CONST char _ctype_b[128 + 256] = {
	_CTYPE_DATA_128_256,
	_CTYPE_DATA_0_127,
	_CTYPE_DATA_128_256
};

#  if defined(__CYGWIN__)
_CONST char __declspec(dllexport) *__ctype_ptr = _ctype_b + 128;
#  else
_CONST char *__ctype_ptr = _ctype_b + 128;
#  endif

#  if defined(_HAVE_ARRAY_ALIASING)

#    if defined(__CYGWIN__)
extern _CONST char __declspec(dllexport) _ctype_[1 + 256] __attribute__ ((alias ("_ctype_b+127")));
#    else
extern _CONST char _ctype_[1 + 256] __attribute__ ((alias ("_ctype_b+127")));
#    endif

#  else /* !_HAVE_ARRAY_ALIASING */

#    if defined(__CYGWIN__)
_CONST char __declspec(dllexport) _ctype_[1 + 256] = {
#    else
_CONST char _ctype_[1 + 256] = {
#    endif
	0,
	_CTYPE_DATA_0_127,
	_CTYPE_DATA_128_256
};
#  endif /* !_HAVE_ARRAY_ALIASING */

#else	/* !defined(ALLOW_NEGATIVE_CTYPE_INDEX) */

# if defined(__CYGWIN__)
_CONST char __declspec(dllexport) _ctype_[1 + 256] = {
# else
_CONST char _ctype_[1 + 256] = {
# endif
	0,
	_CTYPE_DATA_0_127,
	_CTYPE_DATA_128_256
};

_CONST char *__ctype_ptr = _ctype_ + 1;
#endif
