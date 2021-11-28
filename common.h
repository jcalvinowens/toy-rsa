#pragma once

#include <limits.h>

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

#define fatal(...)							\
do {									\
	fprintf(stderr, __FILE__ ":" S__LINE__ ": FATAL: " __VA_ARGS__);\
	abort();							\
} while (0)

#define fatal_on(cond, ...)						\
do {									\
	if (__builtin_expect(cond, 0))					\
		fatal(__VA_ARGS__);					\
} while (0)

#define min(x, y) ({							\
	typeof(x) _min1 = (x);						\
	typeof(y) _min2 = (y);						\
	(void) (&_min1 == &_min2);					\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({							\
	typeof(x) _max1 = (x);						\
	typeof(y) _max2 = (y);						\
	(void) (&_max1 == &_max2);					\
	_max1 > _max2 ? _max1 : _max2; })

#define xchg(x, y)							\
do {									\
	typeof(x) _tmp;							\
	_tmp = x;							\
	x = y;								\
	y = _tmp;							\
} while (0)
