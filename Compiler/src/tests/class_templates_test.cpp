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
			var Point</ SSS /> mut p= zero_init;
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
			var Point</ SSS, i32 /> mut p= zero_init;
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
			var Point</ Box</ i32 /> /> mut p= zero_init;
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
			var Point</ SSS /> mut p= zero_init;
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
			var Point</ std::Box</ i32 /> /> mut p= zero_init;
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
				var Point</ Box</ i32 /> /> mut p= zero_init;
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
					var Point</ Box</ i32 /> /> mut p= zero_init;
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
			var Point</ Box</ f32 /> /> mut p= zero_init;
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
			var Point</ Box</ RR::VV /> /> mut p= zero_init;
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
			var Box</ i32, c_size /> mut p= zero_init;
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
				var Box</ XXX /> mut p= zero_init;
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
				var Box</ T /> mut r= zero_init;
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

U_TEST( ClassTemplateTest17 )
{
	// Recursive deduction preresolve test.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Baz</ T /> { T t; }
		template</ type T /> struct Box</ T /> { T t; } // Must see here

		namespace MMM
		{
			template</ type T />
			struct Point</ Baz</ Box</ T /> /> />
			{
				T x;
			}

			template</ type T /> struct Box</ T /> { T t; } // Not here
		}

		fn Foo() : i32
		{
			var MMM::Point</ Baz</ Box</ i32 /> /> /> mut p= zero_init;
			p.x= 55474;
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

	U_TEST_ASSERT( static_cast<uint64_t>( 55474 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest18_DeduceAlsoTypeParameterFromValueParameter )
{
	static const char c_program_text[]=
	R"(
		template</ type T, T val />
		struct ConstantKeeper</ val />
		{
			fn Get() : T { return val; }
			fn GetZero() : T { return T(0); }
		}

		fn Foo() : u32
		{
			return ConstantKeeper</ 666u />::Get() + ConstantKeeper</ 44u />::GetZero();
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

U_TEST( ClassTemplateTest19_StaticAssertForVariableOfTempateDependentType )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct ZeroInitChecker</ T />
		{
			fn Foo()
			{
				static_assert( T(-0.0f) == T(+0.0f) );
			}
		}

		fn Foo()
		{
			ZeroInitChecker</ i16 />;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassTemplateTest20_OverloadingWithTemplateDependentType )
{
	// Should correctly process functinos and generate class withoute errors (for given arguments).
	static const char c_program_text[]=
	R"(
		template</ type T, type U, type V />
		struct FuncsStroage</ T, U, V />
		{
			fn Foo( T t ){}
			fn Foo( U u ){}
			fn Foo( V v ){}
			fn Foo( i32 i ){}
			fn Foo( i8 i ){}

			fn Baz( T t, U u ){}
			fn Baz( U u, T t ){}
		}

		fn Foo()
		{
			var FuncsStroage</ f32, f64, bool /> fs;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassTemplateTest21_CallOverloadedFunctionWithTemplateDependentSignature )
{
	// Should correctly process calls to overloaded functions with template-dependent parameters in class prepass.
	static const char c_program_text[]=
	R"(
		template</ type T, type U, type V />
		struct FuncsStroage</ T, U, V />
		{
			fn Foo( T t ){}
			fn Foo( U u ){}
			fn Foo( V v ){}
			fn Foo( i32 i ){}
			fn Foo( i8 i ){}

			fn Baz( T t, U u ){}
			fn Baz( U u, T t ){}

			fn Worker()
			{
				Foo( T() );
				Foo( U() );
				Foo( 42.0f );
				Foo( 0u64 );
				Baz( 5, 4.0f );
				Baz( 14.0, 1u8 );
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassTemplateTest22_BoolValueArgument )
{
	static const char c_program_text[]=
	R"(
		template</ bool ret />
		struct BoolReturner</ ret />
		{   fn Get() : bool { return ret; }   }

		fn Foo() : i32
		{
			if( BoolReturner</ true />::Get() && !BoolReturner</ false />::Get() )
			{ return 55412; }
			return 0;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 55412 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest23_SameTemplateArgumentAppearsMultipleTimesInSignature )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct FakePair</ T, T />
		{
			T first; T second;
		}

		fn Foo() : i32
		{
			var FakePair</ i32, i32 /> p{ .first= 58421, .second= 84 };
			return p.first / p.second;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 58421 / 84 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest24_SameTemplateArgumentAppearsInMultipleFormsInSignature )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box{ T t; }

		template</ type T />
		struct FakePair</ T, Box</T/> />
		{
			T first; T second;
		}

		fn Foo() : i32
		{
			var FakePair</ i32, Box</i32/> /> p{ .first= 354154, .second= 65 };
			return p.first / p.second;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 354154 / 65 ) == result_value.IntVal.getLimitedValue() );
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
				var [ i32, 2 ] mut arr= zero_init;
				arr[ loc3 ];

				// Accessing members of T
				loc0.x;
				loc2.func( 42 );
				loc0.func( loc1, loc2, 536.5 );

				// dependent on T lazy logical expression components
				loc0 && true;
				false || loc1;
				loc2 && loc3;

				// dependent on T ++ and --
				++loc0;
				--loc1;

				// dependent on T additive assignment operators
				loc0+= loc1;
				arr[0u]*= loc0;
				loc1+= arr[0u];

				// dependent on T unary operators
				loc1= -loc0;
				loc0= ~loc1;
				!loc2;

				// constexpr for template-dependent stuff
				var i32 constexpr xxx= loc0; // Ok, because not known, constexpr loc0 or not
				var T constexpr yyy= 536; // Ok, because not known, supports T constexpr or not
				auto constexpr zzz= xxx; // Ok, because xxx is template-dependent.

				// Size of array dependent of T
				var [ i32, T(5).x ] mut arr0= zero_init;
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
				if( T() ){} // As codnition
				var T t;
				while(t){} // Variable as condition
				if( T(0) ){} else if( false ) {} // template-dependent expression as condition

				while(!t)
				{
					Baz( T() ); // call function in while block with template-dependent condition.
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

			template</ type T, type F /> struct S</ T, Box</F/> /> { fn Bar() : i32 { return 857; } }
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

U_TEST( PreResolveTest3 )
{
	// In template signature must be visible only template from outer space.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct S</ T /> { fn Bar() : i32 { return 5552; } }

		namespace Baz
		{
			template</ type T />
			struct Box</ S</ T /> />
			{
				fn Worker() : i32
				{
					return S</ T />::Bar();
				}
			}

			template</ type T, type F /> struct S</ T, Box</F/> /> { fn Bar() : i32 { return 441; } }
		}

		fn Foo() : i32
		{
			return Baz::Box</ S</ f32 /> />().Worker();
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

U_TEST( DefaultSignatureArguments_Test0 )
{
	// Second argument is default.
	static const char c_program_text[]=
	R"(
		template</ type T, type P />
		struct S</ T, P= i32 />
		{
			T t; P p;
		}

		fn Foo() : i32
		{
			var S</ f64 /> mut s= zero_init;
			s.p= 42;
			return s.p;
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

U_TEST( DefaultSignatureArguments_Test1 )
{
	// Should select actual arg instead default arg.
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct S</ T= i32 />
		{
			T t;
		}

		fn Foo() : f32
		{
			var S</ f32 /> s{ .t= 3.14f };
			return s.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 3.14f == result_value.FloatVal );
}

U_TEST( DefaultSignatureArguments_Test2 )
{
	// Should select type for default argument, visible at template declaration point.
	static const char c_program_text[]=
	R"(
		struct Box{ i32 x; } // Must be visble

		namespace F
		{
			template</ type T />
			struct S</ T= Box /> // from here,
			{
				T t;
			}

			struct Box{} // nut this must not be visible.
		}

		fn Foo() : i32
		{
			var F::S</ /> s{ .t{ .x= 1536 } };
			return s.t.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1536 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DefaultSignatureArguments_Test3 )
{
	// Default signature argument use already deduced template arguments.
	static const char c_program_text[]=
	R"(
		template</ type T, type Diff />
		struct Vec2</ T, Diff= T />
		{
			T x;
			T y;
			fn GetDiffX( Vec2</ T, Diff /> &imut a, Vec2</ T, Diff /> &imut b ) : Diff
			{
				return Diff(a.x - b.x);
			}
		}

		fn Foo() : i32
		{
			var Vec2</ i32 /> a{ .x= 42, .y=34 }, b{ .y= 12, .x= 12 };
			auto &imut a_ref= a;
			return a_ref.GetDiffX( a, b );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 - 12 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( DefaultSignatureArguments_Test4 )
{
	// Default signature argument use already deduced template arguments.
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T /> { T t; }

		template</ type T, type Boxed />
		struct Wrapper</ T, Boxed= Box</ T /> />
		{
			T x;
			fn GetBoxed( imut this ) : Boxed
			{
				var Boxed r{ .t= x };
				return r;
			}
		}

		fn Foo() : i32
		{
			var Wrapper</ i32 /> w{ .x= 8854 };
			var Box</ i32 /> boxed( w.GetBoxed() );
			return boxed.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8854 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateInsideClass_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			template</ type T /> struct Box</ T /> { T t; }
		}

		fn Foo() : i32
		{
			var A::Box</ i32 /> box{ .t= 588 };
			return box.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 588 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateInsideClass_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			template</ type T /> struct Box</ T /> { T t; }
		}

		template</ type T />
		struct Wrapper</ A::Box</ T /> />     // Should deduce template parameter here.
		{
			T t;
		}

		fn Foo() : i32
		{
			var Wrapper</ A::Box</ i32 /> /> wrapper{ .t= 226587 };
			return wrapper.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 226587 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateInsideClassTemplate_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct A</ T />
		{
			template</ type U />
			struct B</ U />
			{
				T t;
				U u;
			}
		}

		fn Foo() : i32
		{
			var A</ u32 />::B</ f32 /> b{ .t= 584u, .u= 58.128f };
			return i32(b.t) + i32(b.u);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 584 + 58 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateInsideClassTemplate_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct A</ T />
		{
			struct B
			{
				template</ type U />
				struct C</ U />
				{
					T t;
					U u;
				}
			}
		}

		template</ type U />
		struct RevBox</ A</ u32 />::B::C</ U /> /> // Should deduce template inside template
		{
			u32 u;
			U t;
		}

		fn Foo() : i32
		{
			var RevBox</   A</ u32 />::B::C</ f32 />   />  b{ .u= 8954u, .t= -54.128f };
			return i32(b.t) + i32(b.u);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8954 - 54 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateInsideClassTemplate_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct A</ T />
		{
			struct B
			{
				template</ type U />
				struct C</ U />
				{
					T t;
					U u;
				}
			}
		}

		template</ type T />
		struct RevBox</ A</ u32 />::B::C</ T /> /> // Should deduce template inside template
		{
			u32 u;
			T t;
		}

		fn Foo() : i32
		{
			var RevBox</   A</ u32 />::B::C</ f32 />   />  b{ .u= 1584u, .t= 158.128f };
			return i32(b.t) + i32(b.u);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1584 + 158 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ArrayAsClassTemplateParameter_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box</ T />
		{
			T t;
		}

		fn Foo() : i32
		{
			var Box</ [ i32, 4 ] /> box{ .t[ 5, 84, 996, 5412 ] };
			return box.t[0u] + box.t[1u] + box.t[2u] + box.t[3u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 5 + 84 + 996 + 5412 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ShortClassTemplateForm_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Box // Signature arguments will be </ T />
		{
			T t;
		}

		fn Foo() : i32
		{
			var Box</ i32 /> b{ .t= 8842657 };
			return b.t;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 8842657 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ShortClassTemplateForm_Test1 )
{
	// Value parameters in short form.
	static const char c_program_text[]=
	R"(
		template</ type T, u32 size />
		struct ArrayBox // Signature arguments will be </ T, size />
		{
			[ T, size ] t;
		}

		fn Foo() : i32
		{
			var ArrayBox</ i32, 4u /> b{ .t[ 78, 54, 895, 125 ] };
			return b.t[0u] * b.t[1u] + b.t[2u] / b.t[3u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 78 * 54 + 895 / 125 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ShortClassTemplateForm_Test2 )
{
	// Multiple signature arguments.
	static const char c_program_text[]=
	R"(
		template</ type A, type B, type C />
		struct Tuple3 // Signature arguments will be </ A, B, C />
		{
			A first;
			B second;
			C third;
		}

		fn Foo() : i32
		{
			var Tuple3</ i32, f64, u8 /> t{ .first= 8845, .second= 4571.21, .third= 105u8 };
			return ( t.first - i32(t.second) ) * i32(t.third);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( ( 8845 - int(4571.21) ) * 105 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ShortClassTemplateForm_Test3 )
{
	// Value parameter of type of other parameter.
	static const char c_program_text[]=
	R"(
		template</ type T, T size />
		struct ArrayBox // Signature arguments will be </ T, size />
		{
			[ T, size ] t;
		}

		fn Foo() : i32
		{
			var ArrayBox</ i32, 3 /> b{ .t[ -25, -85, 654 ] };
			return b.t[0u] * b.t[1u] + b.t[2u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( (-25) * (-85) + 654 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ShortClassTemplateForm_Test4 )
{
	// Template with zero arguments and short form.
	static const char c_program_text[]=
	R"(
		template</ />
		struct Dummy
		{
			i32 x;
		}

		fn Foo() : i32
		{
			var Dummy</ /> d{ .x= 4125641 };
			return d.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 4125641 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
