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

struct TheClass final
{
	void Method();
	void ImutMethod( TheClass& ) const;

	static void StaticMethod();
	static void StaticMethod2();
};

void TheClass::ImutMethod( TheClass& ) const
{
	std::cout << "Call ImutMethod" << std::endl;
}

void TheClass::StaticMethod2()
{
	std::cout << "Call StaticMethod2" << std::endl;
}

void U32ToStr( unsigned int x, signed char (&str)[64] );

void CondHalt( bool cond );

int main()
{
	// Call some Ü-Sprache functions
	NameSpace::Nested();
	TakeS( NameSpace::S() );
	TheClass the_class;
	the_class.Method();
	the_class.StaticMethod();

	// Real usable function - convert number to string.
	signed char str[64];
	static const unsigned int numbers[]= { 0u, 1u, 5u, 10u, 99u, 100u, 101u, 58u, 586u, 1024u, 1000000000u, 4294967295u };
	for( unsigned int number : numbers )
	{
		U32ToStr( number, str );
		std::cout << str << std::endl;
	}

	const double res= Do( 42 );
	std::cout << "Res: " << res << std::endl;

	CondHalt(false);

	return 0;
}
