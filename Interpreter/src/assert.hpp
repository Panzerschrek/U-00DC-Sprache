#pragma once
#include <cassert>

// Simple assert wrapper.
// If you wish disable asserts, or do something else redefine this macro.
#ifdef U_DEBUG
#define U_ASSERT(x) \
	assert(x)
#else
#define U_ASSERT(x)
#endif

#define U_UNUSED(x) (void)x

// Hack for stupid compilers/ide.
// TODO - use designated intializer, when compilers become more clever.
#if 0
#define U_DESIGNATED_INITIALIZER( index, value ) [ size_t(index) ]= value
#else
// NOW - initializers is not designated - USE SEQUENTIAL initialization.
#define U_DESIGNATED_INITIALIZER( index, value ) value
#endif
