#include <fstream>
#include <iostream>
#include "print.hpp"

static std::ostream& Unwrap(const CppIoStreamRef stream )
{
	return *reinterpret_cast<std::ostream*>(stream);
}

extern "C"
{

CppIoStreamRef U1_GetCerr()
{
	return &std::cerr;
}

void U1_PrintStr( const CppIoStreamRef stream, const char* const string_null_termainated )
{
	Unwrap(stream) << string_null_termainated;
}

void U1_PrintInt( const CppIoStreamRef stream, const int64_t i )
{
	Unwrap(stream) << i;
}

void U1_PrintEndl( const CppIoStreamRef stream )
{
	Unwrap(stream) << std::endl;
}

} // extern "C"
