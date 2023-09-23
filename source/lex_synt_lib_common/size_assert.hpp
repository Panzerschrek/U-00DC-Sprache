#pragma once

// Include this file after stdlib headers in order to check _ITERATOR_DEBUG_LEVEL properly.

#ifdef _MSC_VER
	#ifdef _ITERATOR_DEBUG_LEVEL
		#if _ITERATOR_DEBUG_LEVEL == 0
			#define U_ENABLE_SIZE_ASSERT
		#endif
	#else
		#define U_ENABLE_SIZE_ASSERT
	#endif
#else
	#define U_ENABLE_SIZE_ASSERT
#endif

#ifdef U_ENABLE_SIZE_ASSERT
	#define SIZE_ASSERT( type, min_size ) static_assert( sizeof(type) <= min_size, "Size of " #type " is too big" );
#else
	#define SIZE_ASSERT( type, min_size )
#endif
