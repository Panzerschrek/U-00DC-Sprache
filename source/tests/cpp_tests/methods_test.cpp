#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(MethodTest0)
{
	// Simple declaration of static method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42() : i32 { return 42; }
		}
		fn Foo() : i32
		{
			var S s= zero_init;
			return s.Get42();
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

U_TEST(MethodTest1)
{
	// Should call this method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42( this ) : i32 { return 42; }
		}
		fn Foo() : i32
		{
			var S s= zero_init;
			return s.Get42();
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

U_TEST(MethodTest2)
{
	// Access to struct field via "this".
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetX( this ) : i32 { return this.x; }
		}
		fn Foo() : i32
		{
			var S s{ .x= 84167 };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 84167 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest3)
{
	// Access to struct field directly.
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ i32, 4 ] something_unsignificant_;
			f64 x_;
			fn GetX( this ) : f64 { return x_; }
		}
		fn Foo() : f64
		{
			var S s{ .x_= 84167.1, .something_unsignificant_= zero_init };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 84167.1 == result_value.DoubleVal );
}

U_TEST(MethodTest4)
{
	// Call one class function from another.
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64 { return GetXImpl(); }
			fn GetXImpl( this ) : f64 { return x_; }
		}
		fn Foo() : f64
		{
			var S s{ .x_= -5687.31 };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( -5687.31 == result_value.DoubleVal );
}

U_TEST(MethodTest5)
{
	// Methods overloading
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this, f64 scale ) : f64
			{
				return x_ * scale;
			}
			fn GetX( this, bool negate_it ) : f64
			{
				if( negate_it ) { return -x_; }
				return x_;
			}
		}
		fn Foo() : f64
		{
			var S s{ .x_= 13.0 };
			return s.GetX( 3.0 ) * s.GetX( true );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( ( 13.0 * 3.0 ) * ( -13.0 ) == result_value.DoubleVal );
}

U_TEST(MethodTest6)
{
	// mut/imut this overloading.
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( mut this ) : f64
			{
				return x_ * 2.0;
			}
			fn GetX( imut this ) : f64
			{
				return x_ / 7.0;
			}
		}
		fn  MutRef( S & mut s ) : S & mut { return s; }
		fn IMutRef( S &imut s ) : S &imut { return s; }
		fn Foo() : f64
		{
			var S mut s{ .x_= 13.0 };
			var f64 a= 0.0, b= 0.0;
			return MutRef(s).GetX() - IMutRef(s).GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( ( 13.0 * 2.0 ) - ( 13.0 / 7.0 ) == result_value.DoubleVal );
}

U_TEST(MethodTest7)
{
	// Call one member function from another via explicit "this".
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64 { return this.GetXImpl(); }
			fn GetXImpl( this ) : f64 { return x_; }
		}
		fn Foo() : f64
		{
			var S s{ .x_= 84167.1, };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 84167.1 == result_value.DoubleVal );
}

U_TEST(MethodTes8)
{
	// Prototype for method and realization inside class.
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64;
			fn GetX( this ) : f64 { return x_; }
		}
		fn Foo() : f64
		{
			var S s{ .x_= 84167.1, };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 84167.1 == result_value.DoubleVal );
}

U_TEST(MethodTes9)
{
	// Prototype for method and realization outside class.
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64;
		}
		fn S::GetX( this ) : f64 { return x_; }
		fn Foo() : f64
		{
			var S s{ .x_= 84167.1, };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( 84167.1 == result_value.DoubleVal );
}

U_TEST(MethodTest10)
{
	// Call of static class function without object.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Get42() : i32 { return 42; }
		}
		fn Foo() : i32
		{
			return S::Get42();
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

U_TEST(MethodTest11)
{
	// Test of visibility in members bodies.
	static const char c_program_text[]=
	R"(
		struct X{ i32 x; }
		struct S
		{
			fn GetX( X &imut x ) : i32  // <- here must be visible inner X, because functions processed after inner types.
			{
				return x.y;
			}

			struct X{ i32 y; }
		}

		fn Foo() : i32
		{
			var S::X x{ .y= 55568 };
			return S::GetX( x );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 55568 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest12_ValueArgumentOfCurrentClassInMethod)
{
	static const char c_program_text[]=
	R"(
		struct I
		{
			fn Add( I a, I b ) : I
			{
				var I result { .i( a.i + b.i ) };
				return result;
			}
			i32 i;
		}

		fn Foo() : i32
		{
			var I a{ .i= 584 }, b{ .i= 41257 };
			return I::Add( a, b ).i;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 584 + 41257 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST(MethodTest13_PassThisAsRegularArgumentIntoThisCallFunction)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Set44( S &mut s ) // Static function (non-this-call).
			{
				Set( s, 44 ); // Should call this-call function with argument passed as non-this.
			}

			fn Set( mut this, i32 in_x ){ x= in_x; }

			i32 x;
		}

		fn Foo() : i32
		{
			var S mut s= zero_init;
			S::Set44(s);
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(44) );
}

U_TEST(MethodTest14_PassThisAsRegularArgumentIntoThisCallFunction)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Set( mut this, i32 in_x ){ x= in_x; }

			i32 x;
		}

		fn Foo() : i32
		{
			var S mut s= zero_init;
			S::Set(s, 123); // Call this-call function passing this as regular argument.
			return s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, {} );
	U_TEST_ASSERT( result_value.IntVal.getLimitedValue() == uint64_t(123) );
}

U_TEST( InnerClassTest0 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			struct I
			{
				i32 x;
			}
		}

		fn Foo() : i32
		{
			var S::I i{ .x= 88884 }; // Use inner class.
			return i.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 88884 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( InnerClassTest1 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			struct I
			{
				i32 x;
			}

			fn Foo() : i32
			{
				var I      i0{ .x= 999512 }; // Use inner class, using its name relative this class.
				var ::S::I i1{ .x=  2 }; // Use inner class, using its full name.
				var S::I   i2{ .x= -8 }; // Use inner class, using its partial name.
				return i0.x / i1.x - i2.x;
			}
		}

		fn Foo() : i32
		{
			return S::Foo();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 999512 / 2 - (-8) ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( InnerClassTest2 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			struct I
			{
				class KKK
				{
					fn Foo() : i32 { return 1125894; }
				}
			}
		}

		fn Foo() : i32
		{
			return S::I::KKK::Foo(); // Call static function of inner class.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1125894 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( InnerClassTest3 )
{
	static const char c_program_text[]=
	R"(
		class S
		{
			i32 x; f32 y;

			struct T
			{
				i32 x;
				fn Get( this ) : i32
				{
					// Accessing "this" member, using some different ways.
					return ::S::T::x / 2 - S::T::x / 5 + T::x * 1 + x * 2;
				}
			}
		}

		fn Foo() : i32
		{
			var S::T t {.x= 854 };
			return t.Get();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value=
		engine->runFunction(
			function,
			llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT(
		( 854 / 2 - 854 / 5 + 854 + 854 * 2 ) ==
		static_cast<int32_t>(result_value.IntVal.getLimitedValue()) );
}

} // namespace

} // namespace U
