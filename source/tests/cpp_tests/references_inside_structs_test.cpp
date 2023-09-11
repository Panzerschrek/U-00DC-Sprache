#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( ReferenceClassFieldDeclaration )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 value_field;
			i32 mut mut_value_field;
			i32 imut imut_value_field;

			f32 & ref_field;
			f32 &mut mut_ref_field;
			f32 &imut imut_ref_field;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( BasicReferenceInsideClassUsage )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut r;
		}

		fn Foo() : i32
		{
			auto mut x= 42;
			var S s{ .r= x };
			return s.r;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 42 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( MultipleReferencesInside )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32& x;
			i32& y;
		}

		fn Foo() : i32
		{
			var i32 x= 66, y= 99;
			var S s{ .x= x, .y= y };
			return s.y - s.x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 33 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( MultipleMutableReferencesInside )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			i32 &mut y;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0, mut y= 0;
			{
				var S s{ .x= x, .y= y };
				s.x= 365;
				s.y= 73;
			}
			return x / y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 367 / 73 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( GeneratedCopyConstructorForStructsWithReferencesTest )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut r;
		}

		fn Foo() : i32
		{
			auto mut x= 54745;
			var S s0{ .r= x };
			var S s1( s0 ); // Should correctly call generated copy cosntructor.
			return s1.r;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 54745 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( InitializingReferencesInsideStructsInConstructorInitializerList )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			f32 dummy;
			i32 &mut r;
			fn constructor ( this'x', i32 &'y mut in_r ) ' x <- y '
			( r= in_r, dummy(0.0f) )
			{}
		}

		fn Foo() : i32
		{
			auto mut x= 1124578;
			var S s( x );
			return s.r;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 1124578 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( AccessingReferenceInsideMethodUsingImplicitThis )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn GetX( imut this ) : i32 { return x; }
		}

		fn Foo() : i32
		{
			auto mut x= 954365;
			var S s{ .x= x };
			return s.GetX();
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 954365 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( AssignMutableReferenceInsideClass )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
		}

		fn Foo() : i32
		{
			auto mut x= 0;
			{
				var S s{ .x= x };
				s.x= 54124;
			}
			return x;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 54124 ) == result_value.IntVal.getLimitedValue() );
}

} // namespace

} // namespace U
