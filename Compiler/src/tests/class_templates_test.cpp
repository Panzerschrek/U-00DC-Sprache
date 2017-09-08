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

U_TEST( ClassTemplateTest1 )
{
	// Simple template struct
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Point</ T />
		{
			T x;
			T y;
		}

		fn Foo() : i32
		{
			var Point</ i32 /> p{ .x= 0, .y= 854 };
			return p.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 854 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest2 )
{
	// Multiple instantiation of same class template.
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Point</ T />
		{ T x; T y; }

		fn Square( Point</ i32 /> &imut p ) : i32 // first instantiation
		{
			return p.x * p.x + p.y * p.y;
		}
		fn Foo() : i32
		{
			var Point</ i32 /> p{ .x= -58, .y= 854 }; // second instantiation
			return Square(p);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( (-58) * (-58) + 854 * 854 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest3 )
{
	// Parameter of variable type
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ) : i32 { return x; }

		template</ i32 S />
		struct RRR</ S />
		{
			i32 ss;
			fn constructor()
			( ss= S )
			{
				Bar(S); // We can take address of template value-parameter.
			}
		}

		fn Foo() : i32
		{
			var RRR</ 42 /> rrr;
			return rrr.ss;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest4 )
{
	// Template argument is struct type.
	static const char c_program_text[]=
	R"(
		struct SSS{ i32 a; }

		template</ type T />
		struct Point</ T />
		{
			T x;
			T y;
		}

		fn Foo() : i32
		{
			var Point</ SSS /> p= zero_init;
			p.x.a= 1145874;
			return p.x.a;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1145874 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest5 )
{
	// Partial-specialized template.
	static const char c_program_text[]=
	R"(
		struct SSS{ i32 a; }

		template</ type T />
		struct Point</ SSS, T />
		{
			SSS x;
			SSS y;
		}

		fn Foo() : i32
		{
			var Point</ SSS, i32 /> p= zero_init;
			p.x.a= 1145874;
			return p.x.a;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1145874 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest6 )
{
	// Type deduction from template instance.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }

		template</ type T />
		struct Point</ Box</ T /> />
		{
			T x;
		}

		fn Foo() : i32
		{
			var Point</ Box</ i32 /> /> p= zero_init;
			p.x= 8884;
			return p.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8884 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest7 )
{
	// Fully-specialized template.
	static const char c_program_text[]=
	R"(
		struct SSS{ i32 a; }

		template</ />
		struct Point</ SSS /> { SSS x;}

		fn Foo() : i32
		{
			var Point</ SSS /> p= zero_init;
			p.x.a= 1145874;
			return p.x.a;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1145874 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest8 )
{
	// Type deduction from template instance. Template placed inside namespace.
	static const char c_program_text[]=
	R"(
		namespace std
		{
			template</ type T /> struct Box</ T /> { T t; }
		}

		template</ type T />
		struct Point</ std::Box</ T /> />
		{
			T x;
		}

		fn Foo() : i32
		{
			var Point</ std::Box</ i32 /> /> p= zero_init;
			p.x= 666;
			return p.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 666 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest9 )
{
	// Type deduction from template instance. First template placed in outer namespace, relative to second template.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }

		namespace NNN
		{
			template</ type T />
			struct Point</ Box</ T /> />
			{
				T x;
			}

			fn Foo() : i32
			{
				var Point</ Box</ i32 /> /> p= zero_init;
				p.x= 9998565;
				return p.x;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN3NNN3FooEv" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 9998565 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest10 )
{
	// Type deduction from template instance. Use struct from template class.
	static const char c_program_text[]=
	R"(
		template</ type T /> class Baz</ T />
		{
			struct Feed{ T t; }
		}

		template</ type T />
		struct Point</ Baz</ T />::Feed />
		{
			T x;
		}

		fn Foo() : f32
		{
			var Point</ Baz</ f32 />::Feed /> p= zero_init;
			p.x= -3.14f;
			return p.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( -3.14f == result_value.FloatVal );
}

U_TEST( ClassTemplateTest11 )
{
	// Type deduction from template instance. Both templates placed inside common namespace.
	static const char c_program_text[]=
	R"(
		namespace Br
		{
			template</ type T /> struct Box</ T /> { T t; }

			namespace NNN
			{
				template</ type T />
				struct Point</ Box</ T /> />
				{
					T x;
				}

				fn Foo() : i32
				{
					var Point</ Box</ i32 /> /> p= zero_init;
					p.x= 1996;
					return p.x;
				}
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN2Br3NNN3FooEv" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1996 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest12 )
{
	// Type deduction from template instance. Type is given.
	static const char c_program_text[]=
	R"(
		template</ type T /> class Box</ T />
		{
			T t;
		}

		template</  />
		struct Point</ Box</ f32 /> />
		{
			f32 x;
		}

		fn Foo() : f32
		{
			var Point</ Box</ f32 /> /> p= zero_init;
			p.x= -3.14f;
			return p.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( -3.14f == result_value.FloatVal );
}

U_TEST( ClassTemplateTest13 )
{
	// Type deduction from template instance. Type is given and placed in namespace.
	static const char c_program_text[]=
	R"(
		namespace RR{ struct VV{} }

		template</ type T /> class Box</ T />
		{
			T t;
		}

		template</  />
		struct Point</ Box</ RR::VV /> />
		{
			f32 x;
		}

		fn Foo() : f32
		{
			var Point</ Box</ RR::VV /> /> p= zero_init;
			p.x= -3.14f;
			return p.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( -3.14f == result_value.FloatVal );
}

} // namespace U
