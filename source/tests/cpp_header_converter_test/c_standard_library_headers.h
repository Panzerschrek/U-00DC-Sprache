#include <assert.h>
#include <complex.h>
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdatomic.h>
// #include <stdbit.h> // C23
#include <stdbool.h>
// #include <stdckdint.h> // C23
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#ifndef _MSC_VER // tgmath for MSVC doesn't work properly.
	#include <tgmath.h>
#endif
// #include <threads.h> // C11, but unsupported by GCC.
#include <time.h>
#include <uchar.h>
#include <wchar.h>
#include <wctype.h>
