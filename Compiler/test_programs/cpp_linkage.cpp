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
	void ImutMethod() const;

	static void StaticMethod();
	static void StaticMethod2();
};

void TheClass::ImutMethod() const
{
	std::cout << "Call ImutMethod" << std::endl;
}

void TheClass::StaticMethod2()
{
	std::cout << "Call StaticMethod2" << std::endl;
}

int main()
{
	// Call some Ü-Sprache functions
	NameSpace::Nested();
	TakeS( NameSpace::S() );
	TheClass the_class;
	the_class.Method();
	the_class.StaticMethod();

	const double res= Do( 42 );
	std::cout << "Res: " << res << std::endl;
	return 0;
}
