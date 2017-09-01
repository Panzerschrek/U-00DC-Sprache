#include "tests.hpp"

namespace U
{

U_TEST( ClassTemplateTest0 )
{
	// Simple declaration of class templates.
	static const char c_program_text[]=
	R"(
		template</ type T /> // one type parameter
		class A</ T /> {}

		template</ /> // zero parameteres
		class B</ />  {}

		template</ i32 count /> // one value paramerer
		class C</ count />  {}

		template</ type I, I count /> // Value parameter, dependent on type parameter.
		class D</ count />  {}

		template</ type I, type J /> // multiple type parameteres
		class E</ J, I />  {}

		template<//> // Template with zero arguments and nonzero type signature arg.
		class F</ i32 />   {}
	)";

	BuildProgram( c_program_text );
}

} // namespace U
