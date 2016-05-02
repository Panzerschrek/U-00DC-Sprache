#pragma once
#include <cassert>

// Simple assert wrapper.
// If you wish disable asserts, or do something else redefine this macro.
#define U_ASSERT(x) \
	assert(x)

#define U_UNUSED(x) (void)x
