#include <cstdint>

extern "C"
{

// If contents of this file changed, print.uh must be changed too!

// Print to std::cerr

void U1_PrintStr( const char* string_null_termainated );
void U1_PrintInt( int64_t i );
void U1_PrintEndl();

} // extern "C"
