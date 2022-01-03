#include <iostream>

// Imported from Ü-Sprache func.
double Do( int x );

// Function, exported to Ü-Sprache
void CallCPP( int x )
{
	std::cout << "Call " << x << " CPP!" << std::endl;
}

namespace NameSpace
{
	extern void Nested();

	struct S{};
}

extern void TakeS( const NameSpace::S& );

void U32ToStr( unsigned int x, char (&str)[64] );

void CondHalt( bool cond );

extern "C" int NoMangleFunction();

extern "C" int NoMangleFunctionCPPSide()
{
	return 1917;
}

int main()
{
	// Call some Ü-Sprache functions
	NameSpace::Nested();
	TakeS( NameSpace::S() );

	// Real usable function - convert number to string.
	char str[64];
	static const unsigned int numbers[]= { 0u, 1u, 5u, 10u, 99u, 100u, 101u, 58u, 586u, 1024u, 1000000000u, 4294967295u };
	for( unsigned int number : numbers )
	{
		U32ToStr( number, str );
		std::cout << str << std::endl;
	}

	const double res= Do( 42 );
	std::cout << "Res: " << res << std::endl;
	std::cout << "NoMangle res: " << NoMangleFunction() << std::endl;

	CondHalt(false);

	return 0;
}
