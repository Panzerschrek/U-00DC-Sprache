// Disable some warnings from llvm headers.

#ifdef __GNUC__
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmismatched-tags"
#endif // __GNUC__

#ifdef _MSC_VER
#pragma warning( push )

#pragma warning( disable : 4141 )
#pragma warning( disable : 4146 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4291 )
#pragma warning( disable : 4309 )
#pragma warning( disable : 4624 )
#endif // _MSC_VER
