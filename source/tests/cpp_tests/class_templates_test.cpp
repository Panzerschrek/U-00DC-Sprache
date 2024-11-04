#include "cpp_tests.hpp"

namespace U
{

namespace
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
				static_assert( T(-0.0f) == T(0.0f) );
			}
		}

		fn Foo()
		{
			type T= ZeroInitChecker</ i16 />;
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

U_TEST( ClassTemplateTest25_SameTypeTemplateArgumentAppearsMultipleTimes )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct S</ T, T />{ T x; T y; }

		fn Foo() : i32
		{
			var S</ i32, i32 /> s{ .x= 56, .y= 3 };
			return s.x / s.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 56 / 3 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ClassTemplateTest26_SameValueTemplateArgumentAppearsMultipleTimes )
{
	static const char c_program_text[]=
	R"(
		template</ i32 init />
		struct S</ init, init />
		{
			i32 x; i32 y;
			fn constructor() ( x= init, y= init ) {}
		}

		fn Foo() : i32
		{
			var S</ 59, 59 /> s;
			return s.x * s.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );
	U_TEST_ASSERT( static_cast<uint64_t>( 59 * 59 ) == result_value.IntVal.getLimitedValue() );
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

				// dependent on T compound assignment operators
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

U_TEST( ClassPrepass_Test3 )
{
	// Class repass and inheritance.
	static const char c_program_text[]=
	R"(
		class I polymorph{}
		fn Foo( I& i ){}
		template</ type T />
		struct X
		{
			fn Bar()
			{
				var T t;
				Foo(t); // Ok, converting template-dependent type to other type.
				var I& i= t; // Ok, converting template-dependent reference to other reference.
			}
		}

		template</ type T />
		class Y : T
		{
			type ThisType= Y</T/>;
			fn Baz( ThisType& y ){}
			fn Bar( mut this )
			{
				Foo(this); // Ok, type of this have template-dependent parent.
				var I i;
				Baz(i); // Ok, converting reference of known type to reference of this type, because this type have template-dependent parents.
			}
		}
	)";

	BuildProgram( c_program_text );
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

U_TEST( DefaultSignatureArguments_Test5 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct Box</ T = i32 /> { type TT = T; }

		template</ />
		struct Wrapper</ Box</ /> />   // Works, like Box</ i32 />
		{
			type TT= f32;
		}

		fn Foo() : i32
		{
			var Wrapper</ Box</ /> />::TT pi= 3.1415926535f;
			var Wrapper</ Box</ i32 /> />::TT e= 2.718281828f;
			return i32(pi * 1000.0f) + i32(e * 1000.0f);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 3141 + 2718 ) == result_value.IntVal.getLimitedValue() );
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

U_TEST( NumericConstantAsTemplateSignatureParameter_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct IntVec</ T, 2 />
		{
			T x; T y;
		}

		fn Foo() : i32
		{
			var IntVec</ i32, 1 + 1 /> v{ .x= 23214, .y= 2221 };
			return v.x - v.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 23214 - 2221 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( NumericConstantAsTemplateSignatureParameter_Test1 )
{
	static const char c_program_text[]=
	R"(
		var i32 constexpr two= 2;
		template</ type T />
		struct IntVec</ T, two /> // constexpr variable as specialized signature parameter
		{
			T x; T y;
		}

		fn Foo() : i32
		{
			var IntVec</ i32, 2 /> v{ .x= 9987, .y= 1124 };
			return v.x - v.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 9987 - 1124 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( NumericConstantAsTemplateSignatureParameter_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ i32 default_val />
		struct Box
		{
			i32 x;
			fn constructor() ( x= default_val ){}
			fn constructor( i32 in_x ) ( x= in_x ){}
		}

		template</ />
		struct ZeroBoxVec</ Box</ 0 /> />   // numeric constant is inside template signature arg.
		{
			Box</ 0 /> x;
			Box</ 0 /> y;
		}

		fn Foo() : i32
		{
			var ZeroBoxVec</ Box</ 0 /> /> v{ .x( 654 ), .y( 114 ) };
			return v.x.x - v.y.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 654 - 114 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( VariableExpressionAsTemplateSignatureParameter_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct IntVec</ T, ( 1 + 1 ) * 1 />
		{
			T x; T y;
		}

		fn Foo() : i32
		{
			var IntVec</ i32, 2 /> v{ .x= 654740, .y= 66523 };
			return v.x - v.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 654740 - 66523 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( VariableExpressionAsTemplateSignatureParameter_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Vec</ T, -( 0 - 2 ) />  // expression with brackets and unary minus
		{
			T x; T y;
		}

		fn Foo() : i32
		{
			var Vec</ i32, 2 /> v{ .x= 7451, .y= 1142 };
			return v.x - v.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 7451 - 1142 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( BracketExpressionAsTemplateSignatureParameter_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T />
		struct Vec2</ (T) />
		{
			T x; T y;
		}

		fn Foo() : i32
		{
			var Vec2</ i32 /> v{ .x= 6541, .y= 214 };
			return v.x / v.y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 6541 / 214 ) == result_value.IntVal.getLimitedValue() );
}


U_TEST( ArrayAsTemplateSignatureParameter_Test0 )
{
	static const char c_program_text[]=
	R"(
		// Array size is constant
		template</ type T />
		struct ArrayElement</ [ T, size_type(4) ] />
		{
			T x;
		}

		fn Foo() : i32
		{
			var ArrayElement</ [ i32, 4u ] /> el{ .x= 584144 };
			return el.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 584144 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ArrayAsTemplateSignatureParameter_Test1 )
{
	static const char c_program_text[]=
	R"(
		// Array size is param.
		template</ type T, size_type size />
		struct ArrayWrapper</ [ T, size ] />
		{
			[ T, size ] x;
		}

		fn Foo() : i32
		{
			var ArrayWrapper</ [ i32, 2u ] /> arr{ .x[ 85647, 32141 ] };
			return arr.x[0u] - arr.x[1u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 85647 - 32141 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ArrayAsTemplateSignatureParameter_Test2 )
{
	static const char c_program_text[]=
	R"(
		// Array size is param, type is known.
		template</ size_type size />
		struct ArrayWrapper</ [ i32, size ] />
		{
			[ i32, size ] x;
		}

		fn Foo() : i32
		{
			var ArrayWrapper</ [ i32, 3u ] /> arr{ .x[ 8547, 3214, 11 ] };
			return ( arr.x[0u] - arr.x[1u] ) * arr.x[2u];
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( ( 8547 - 3214 ) * 11 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( LazyClassFunctionsBuild_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</type T/>
		struct A
		{
			fn Foo(this, u32 x)
			{
				if(x > 0u)
				{
					B</T/>().Bar(x-1u);
				}
			}
		}

		template</type T/>
		struct B
		{
			fn Bar(this, u32 x)
			{
				if(x > 0u)
				{
					A</T/>().Foo(x-1u);
				}
			}
		}

		type Ai= A</i32/>;
		type Bi= B</i32/>;
	)";

	BuildProgram( c_program_text );
}

U_TEST( TemplateClass_UseLocalVariableForTemplateArgumentUsedAsReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Baz(i32& x){}

		template</i32 x/>
		struct S
		{
			fn Bar()
			{
				Baz(x);  // Pass reference to argument "x". "x" must be global variable.
			}
		}
		fn Foo()
		{
			auto some_local= 666;
			S</some_local/>::Bar(); // Use simple form of template signature.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );
}

U_TEST( TemplateClass_UseLocalVariableForTemplateArgumentUsedAsReference_Test1 )
{
	static const char c_program_text[]=
	R"(
		fn Baz(i32& x){}

		template</i32 x/>
		struct S</x/>
		{
			fn Bar()
			{
				Baz(x);  // Pass reference to argument "x". "x" must be global variable.
			}
		}
		fn Foo()
		{
			auto some_local= 999;
			S</some_local/>::Bar(); // Use extended form of template signature.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );
}

U_TEST( TemplateClass_UseLocalVariableForTemplateArgumentUsedAsReference_Test2 )
{
	static const char c_program_text[]=
	R"(
		fn Baz(size_type& x){}

		template</size_type x/>
		struct S</ [ f32, x ] />
		{
			fn Bar()
			{
				Baz(x);  // Pass reference to argument "x". "x" must be global variable.
			}
		}
		fn Foo()
		{
			auto some_local= 1234s;
			S</ [ f32, some_local ] />::Bar(); // Use extended form of template signature with array type.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );
	engine->runFunction( function, {} );
}

U_TEST( ClassTemplateWithZeroParams_Test0 )
{
	static const char c_program_text[]=
	R"(
		// struct template with zero args is lazy.
		template<//> struct Stupid
		{
			UnknownTypeName field; // No error is generated, since this class template is not instantiated.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ClassTemplateWithZeroParams_Test1 )
{
	static const char c_program_text[]=
	R"(
		template<//> struct Box
		{
			i32 x;
		}
		fn Foo() : i32
		{
			var Box<//> b { .x= 678 }; // Instantiate zero-args struct template here.
			return b.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(678) );
}

U_TEST( ClassTemplateWithZeroParams_Test2 )
{
	static const char c_program_text[]=
	R"(
		template<//> struct Box</ f64 /> // Zero params, but non-zero signature args.
		{
			i32 x;
		}
		fn Foo() : i32
		{
			var Box</ f64 /> b { .x= 876 }; // Instantiate zero-params struct template here.
			return b.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(876) );
}

} // namespace

} // namespace U
