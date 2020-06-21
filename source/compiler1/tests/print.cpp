#include <iostream>
#include "print.hpp"

extern "C"
{

void U1_PrintStr( const char* const string_null_termainated )
{
	std::cerr << string_null_termainated;
}

void U1_PrintInt( const int64_t i )
{
	std::cerr << i;
}

void U1_PrintEndl()
{
	std::cerr << std::endl;
}

} // extern "C"
