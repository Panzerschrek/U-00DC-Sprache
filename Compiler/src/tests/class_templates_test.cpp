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

U_TEST( ClassTemplateTest14 )
{
	// Use local constexpr variable as template value-argument.
	static const char c_program_text[]=
	R"(
		template</ type T, u32 size />
		struct Box</ T, size />
		{
			[ T, size ] x;
		}

		fn Foo() : i32
		{
			auto constexpr c_size= 14u;
			var Box</ i32, c_size /> p= zero_init;
			p.x[13u]= 2017;
			return p.x[13u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 2017 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest15 )
{
	// Use type for template argument, not visible in template space, but visible in instantiation point.
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box</ T /> { T t; }

		namespace DDR
		{
			struct XXX{ u32 x; }
			fn Foo() : u32
			{
				var Box</ XXX /> p= zero_init;
				p.t.x= 733u;
				return p.t.x;
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_ZN3DDR3FooEv" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 733 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest16 )
{
	// Reference to self-type inside class template.
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box</ T />
		{
			T t;

			fn Foo() : f64
			{
				var Box</ T /> r= zero_init;
				r.t= T(458);
				return r.t;
			}
		}

		fn Foo() : u32
		{
			var Box</ f64 /> p= zero_init;
			return u32(p.Foo());
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 458 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassPrepass_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Baz( f32 x ){}
		template</ type T />
		class CC</ T />
		{
			fn Ret( i32 x ) : T
			{
				// Some initializers.
				var T loc0= zero_init;
				var T loc1(45, 58);
				var T loc2{ .x= 45, .y= 34 };
				var T loc3= 45 / 2;

				loc3( 45 ); // dependent on T call operator.
				Baz( loc1 ); // dependent on T argument.
				loc0[42u]; // dependent on T indexation

				// dependent on T indexation
				var [ i32, 2 ] arr= zero_init;
				arr[ loc3 ];

				// Accessing members of T
				loc0.x;
				loc2.func( 42 );
				loc0.func( loc1, loc2, 536.5 );

				// dependent on T lazy logical expression components
				loc0 && true;
				false || loc1;
				loc2 && loc3;

				// constexpr for template-dependent stuff
				var i32 constexpr xxx= loc0; // Ok, because not known, constexpr loc0 or not
				var T constexpr yyy= 536; // Ok, because not known, supports T constexpr or not
				auto constexpr zzz= xxx; // Ok, because xxx is template-dependent.

				// Size of array dependent of T
				var [ i32, T(5).x ] arr0= zero_init;
				arr0[45u]= 55; // Size check must not work, because array size is undefined
				var [ i32, xxx ] arr1[ 5, 6, 7, 8, 8, 8, 2564, 846 ]; // Initializer count check must not work, because size is undefined
				var [ i32, 2 ] arr2= zero_init;
				arr[ u32(xxx) ]= 58; // Size check must not work, because index is undefined
				var [ i32, T(7) ] constexpr arr3[];
				auto constexpr arr3_member= arr3[1u]; // Constexpr value is undefined, because initializer is undefined, because it depends on T

				x + T( 0.0f ); // dependent on T temporary variable construction.
				return x; // dependent on T return.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassPrepass_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Baz( f32 x ){}
		template</ type T />
		class CC</ T />
		{
			fn Ret( i32 x ) : T
			{
				// Should work correctly with "if/else", "while" with template-dependent value as arguments.
				if( T ){} // As codnition
				var T t;
				while(t){} // Variable as condition
				if( T(0) ){} else if( false ) {} // template-dependent expression as condition

				while(!t)
				{
					Baz( T ); // call function in while block with template-dependent condition.
					Baz( 0.0f );
				}
				return 0;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassPrepass_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> class CC</ T />{}
		template</ type T />
		class Baz</ T />
		{
			fn Foo() : i32
			{
				// instantiation of template with tempate-dependent value as signature argument is template-dependent value
				var CC</ T /> cc;
				cc.Foo();
				cc + 42;
				cc.x.y= 4447.0f;
				return cc;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( PreResolveTest0 )
{
	// Inside template must be visible only one of two overloaded functions.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ) : i32 { return 5; }

		template</ type T />
		struct Box</ T />
		{
			fn Worker() : i32
			{
				var i32 x= 0;
				return Bar(x);
			}
		}

		fn Bar( i32 &mut x ) : i32 { return 536; }

		fn Foo() : i32
		{
			var Box</f32/> b;
			return b.Worker();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( PreResolveTest1 )
{
	// Inside template must be visible only function from outer space, but not function from inner space, defined later.
	static const char c_program_text[]=
	R"(
		fn Bar() : i32 { return 6666; }

		namespace Baz
		{
			template</ type T />
			struct Box</ T />
			{
				fn Worker() : i32
				{
					return Bar();
				}
			}

			fn Bar() : i32 { return 42; }
		}

		fn Foo() : i32
		{
			return Baz::Box</f32/>().Worker();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 6666 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( PreResolveTest2 )
{
	// Inside template must be visible only class template from outer space, but not class template from inner space, defined later.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct S</ T /> { fn Bar() : i32 { return 5552; } }

		namespace Baz
		{
			template</ type T />
			struct Box</ T />
			{
				fn Worker() : i32
				{
					return S</ i32 />().Bar();
				}
			}

			template</ type T, type F /> struct S</ T, Box</F/> /> { fn Bar() : i32 { return 5552; } }
		}

		fn Foo() : i32
		{
			return Baz::Box</f32/>().Worker();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5552 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
