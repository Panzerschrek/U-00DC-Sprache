#include <fstream>
#include <iostream>

using CppIoStreamRef= void*;

static std::ostream& Unwrap(const CppIoStreamRef stream )
{
	return *reinterpret_cast<std::ostream*>(stream);
}

extern "C"
{

// If contents of this file changed, print.uh must be changed too!

// Print to given stream

CppIoStreamRef U1_GetCerr()
{
	return &std::cerr;
}

void U1_PrintStr( const CppIoStreamRef stream, const char* const string_null_termainated )
{
	Unwrap(stream) << string_null_termainated;
}

} // extern "C"
