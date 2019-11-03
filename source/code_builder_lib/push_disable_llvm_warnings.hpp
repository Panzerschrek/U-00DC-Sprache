#ifdef __GNUC__

// Disable some warnings from llvm headers.

#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"

#endif

#ifdef _MSC_VER

#pragma warning( push )

#pragma warning( disable : 4624 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )

#endif
