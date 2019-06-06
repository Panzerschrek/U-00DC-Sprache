#ifdef __GNUC__

// Disable some warnings from boost headers.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#if __GNUC__ >= 7
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#endif
