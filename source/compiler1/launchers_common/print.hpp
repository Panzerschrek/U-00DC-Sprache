#include <cstdint>

extern "C"
{

// If contents of this file changed, print.uh must be changed too!

// Print to given stream

using CppIoStreamRef= void*;

CppIoStreamRef U1_GetCerr();

void U1_PrintStr( CppIoStreamRef stream, const char* string_null_termainated );
void U1_PrintInt( CppIoStreamRef stream, int64_t i );
void U1_PrintEndl( CppIoStreamRef stream );

} // extern "C"
