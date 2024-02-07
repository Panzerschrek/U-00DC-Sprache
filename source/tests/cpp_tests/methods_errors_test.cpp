#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( BindingConstReferenceToNonconstReference_InThisCall_Test0 )
{
	// Call of nonstatic "thiscall" method with mutable this, using immutable object.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Do( mut this ){}
		}
		fn Foo()
		{
			var S imut s{};
			s.Do();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( BindingConstReferenceToNonconstReference_InThisCall_Test1 )
{
	// Call of nonstatic "thiscall" method with mutable this, from method, where "this" is immutable.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn DoImpl( mut this ){}
			fn Do( imut this )
			{
				DoImpl();
			}
		}
		fn Foo()
		{
			var S s{};
			s.Do();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ClassFieldAccessInStaticMethodTest0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			f32 x_;
			fn Do() : f32
			{
				return x_;
			}
		}
		fn Foo()
		{
			var S imut s{};
			s.Do();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ClassFieldAccessInStaticMethod, 7u ) );
}

U_TEST(FunctionBodyDuplication_ForMethods_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64 { return x_; }
			fn GetX( this ) : f64 { return -x_; }
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(FunctionPrototypeDuplication_ForMethods_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			f64 x_;
			fn GetX( this ) : f64;
			fn GetX( this ) : f64;
			fn GetX( this ) : f64 { return -x_; }
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(ThisInNonclassFunctionTest0)
{
	static const char c_program_text[]=
	R"(
		fn Bar( this, i32 x ){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisInNonclassFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(ThiscallMismatch_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Foo(this);
			fn Foo( S& s ){}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThiscallMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u || error.src_loc.GetLine() == 5u );
}

U_TEST(ThiscallMismatch_Test1)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Foo( S& s );
		}
		fn S::Foo( this ){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThiscallMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( AccessOfNonThisClassFieldTest0 )
{
	static const char c_program_text[]=
	R"(
		struct A
		{
			f32 x;

			struct B
			{
				fn Method( this )
				{
					// Access "x", which is member of parent class.
					x; // simple name
					::A::x; // full name
					A::x; // partial name
				}
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.size() >= 3 );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::AccessOfNonThisClassField );
	U_TEST_ASSERT( build_result.errors[0].src_loc.GetLine() == 11u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::AccessOfNonThisClassField );
	U_TEST_ASSERT( build_result.errors[1].src_loc.GetLine() == 12u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::AccessOfNonThisClassField );
	U_TEST_ASSERT( build_result.errors[2].src_loc.GetLine() == 13u );
}

U_TEST(ThisUnavailableTest0)
{
	// "this" in nonclass fnction.
	static const char c_program_text[]=
	R"(
		fn Bar(){ this; }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisUnavailable );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(ThisUnavailableTest1)
{
	// "this" in static method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Bar()
			{
				this;
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ThisUnavailable );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(TryCallThisCallFunctionPassingExplicitThisWhenImpliciThisAlsoExists)
{
	// "this" in static method.
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Foo( this )
			{
				Bar( this ); // Error, can't call "Bar" because actually two arguments passed - implicit and explicit "this".
			}

			fn Bar( this ){}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

} // namespace

} // namespace U
