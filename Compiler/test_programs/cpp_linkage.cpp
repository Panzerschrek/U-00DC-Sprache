#include <iostream>

// SPRACHE_TODO - tune mangling and remove "extern C" trash.
// In result, we must have same mangling, as C++.

// Imported from Ü-Sprache func.
extern "C" double _Z2Do( int x );

// Function, exported to Ü-Sprache
extern "C" void _Z7CallCPP( int x )
{
	std::cout << "Call " << x << " CPP!" << std::endl;
}

int main()
{
	const double res= _Z2Do( 42 );
	std::cout << "Res: " << res << std::endl;
	return 0;
}
