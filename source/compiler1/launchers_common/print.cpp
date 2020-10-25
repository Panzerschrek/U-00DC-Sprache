#include <fstream>
#include <iostream>
#include "print.hpp"

extern "C"
{

static std::ostream& Unwrap(const CppIoStreamRef stream )
{
	return *reinterpret_cast<std::ostream*>(stream);
}

CppIoStreamRef U1_GetCout()
{
	return &std::cout;
}

CppIoStreamRef U1_GetCerr()
{
	return &std::cerr;
}

CppIoStreamRef U1_GetNullStream()
{
	static std::ofstream dummy_stream;
	dummy_stream.setf(std::ios_base::fmtflags(std::ios::badbit));
	return &static_cast<std::ostream&>(dummy_stream);
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
