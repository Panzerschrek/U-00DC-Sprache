#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(NameNotFoundTest_Minus1)
{
	// Unknown type.
	static const char c_program_text[]=
	R"(
		fn Foo() : unknown_type
		{
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(NameNotFoundTest0)
{
	// Unknown named operand.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return y;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 4u ) );
}

U_TEST(NameNotFoundTest1)
{
	// Unknown type name.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var UnknownType x= 0;
			return 42;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(NameNotFoundTest2)
{
	// Unknown member name.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo() : i32
		{
			var S x;
			return x.unexistent_field;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 6u ) );
}

U_TEST(NameNotFoundTest3)
{
	// Unknown name fetched from the root namespace.
	static const char c_program_text[]=
	R"(
		type T= ::some_unknown_global;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 2u ) );
}

U_TEST(UsingKeywordAsName0)
{
	// Function name is keyword.
	static const char c_program_text[]=
	R"(
		fn var() : i32
		{
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(UsingKeywordAsName1)
{
	// Arg name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 continue ) : i32
		{
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(UsingKeywordAsName2)
{
	// struct name is keyword.
	static const char c_program_text[]=
	R"(
		struct while{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(UsingKeywordAsName3)
{
	// Variable name is keyword.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 void= 0;
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(UsingKeywordAsName4)
{
	// Argument with name "this", that is not actual "this".
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn Foo( i32 this );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(UsingKeywordAsName5)
{
	// Class field
	static const char c_program_text[]=
	R"(
		struct S
		{
			u32 type;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(UsingKeywordAsName6)
{
	// auto variable
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto virtual= 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(UsingKeywordAsName7)
{
	// auto global variable
	static const char c_program_text[]=
	R"(
		auto await= 0;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(UsingKeywordAsName8)
{
	// global variable
	static const char c_program_text[]=
	R"(
		var i32 override= 0;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UsingKeywordAsName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(Redefinition0)
{
	// Variable redefinition in same scope.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 x= 0;
			var i32 x= 0;
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(Redefinition1)
{
	// Variable redefinition in different scopes.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			var i32 x= 0;
			{ var i32 x= 0; }
			return 0;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST(Redefinition3)
{
	// Function redefinition.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{ return 0; }

		fn Bar() : i32
		{ return 1; }

		struct Foo{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 8u );
}

U_TEST( Redefinition4 )
{
	static const char c_program_text[]=
	R"(
		namespace A
		{
			struct S{}
		}
		namespace B
		{
			type LocalS= A::S;
			struct LocalS{} // Error, redefine type alias
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( Redefinition5 )
{
	// Arg redefinition
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x, f32 x ){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::Redefinition );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( Redefinition6 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x )
		{
			var i32 x= 0; // Redefine argument in root function block.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::Redefinition, 4u ) );
}

U_TEST( Redefinition7 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x )
		{
			{
				var i32 x= 0; // Ok - redefine argument in inner block.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition8 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			with( some_var : 444 )
			{
				var i32 z= 0;
				auto some_var= 0; // Redefine "with" operator variable.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::Redefinition, 7u ) );
}

U_TEST( Redefinition9 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			with( some_var : 444 )
			{
				{
					auto some_var= 0; // Ok - redefine "with" operator variable in inner block.
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition10 )
{
	static const char c_program_text[]=
	R"(
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen )
			{
				auto x= 0; // Redefine "if_coro_advance" operator variable.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::Redefinition, 8u ) );
}

U_TEST( Redefinition11 )
{
	static const char c_program_text[]=
	R"(
		fn generator SomeGen() : i32;
		fn Foo()
		{
			auto mut gen= SomeGen();
			if_coro_advance( x : gen )
			{
				{
					auto x= 0; // Ok - redefine "if_coro_advance" operator variable in inner block.
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition12 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( tup[f32, i32]& t )
		{
			for( v : t )
			{
				auto v= 0; // Redefinition of tuple-for variable.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::Redefinition, 6u ) );
}

U_TEST( Redefinition13 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( tup[f32, i32]& t )
		{
			for( v : t )
			{
				{
					auto v= 0; // Ok - redefinition of tuple-for variable inside block.
				}
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition14 )
{
	// Branches of "if" use separate name scopes.
	static const char c_program_text[]=
	R"(
		fn Foo( bool cond )
		{
			var i32 x= 0;
			if( cond )
			{
				auto x= 0.25f;
			}
			else
			{
				var [ bool, 4 ] x= zero_init;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition15 )
{
	// Branches of "static_if" use separate name scopes.
	static const char c_program_text[]=
	R"(
		template</bool cond/> fn Cond()
		{
			var i32 x= 0;
			static_if( cond )
			{
				auto x= 0.25f;
			}
			else
			{
				var [ bool, 4 ] x= zero_init;
			}
		}
		fn Foo()
		{
			Cond</false/>();
			Cond</true/>();
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition16 )
{
	static const char c_program_text[]=
	R"(
		fn Bar() : bool;
		fn Foo()
		{
			var i32 x= 0;
			while(Bar())
			{
				var bool x= false; // Ok, "while" creates separate names scope.
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( Redefinition17 )
{
	// C-style for operator creates two name scopes - one for own variables and one for internal block.
	// Second scope is needed in order to destruct properly all variables, created in iteration block and avoid
	// names from it to be visible in iteration part.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			auto mut i= 0;
			for( var size_type mut s(0); s < 10s; ++s )
			{
				auto s= 0.12345f;
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST(UnknownNumericConstantTypeTest0)
{
	// unknown name
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 45hz;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UnknownNumericConstantType, 4u ) );
}

U_TEST(UnknownNumericConstantTypeTest1)
{
	// existent type name in upper case
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 45I32;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UnknownNumericConstantType, 4u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest0)
{
	// Binary operations errors.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var S s= zero_init;
			var [ i32, 5 ] arr= zero_init;
			false + true; // No binary operators for booleans.
			1u8 - 4u8; // Operation not supported for small integers.
			arr * arr; // Operation not supported for arrays.
			0.35f32 & 1488.42f32; // Bit operator for floats.
			false > true; // Comparision of bools.
			arr <= arr; // Comparision of arrays.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 6u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].src_loc.GetLine() == 7u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].src_loc.GetLine() == 8u );
	U_TEST_ASSERT( build_result.errors[2].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[2].src_loc.GetLine() == 9u );
	U_TEST_ASSERT( build_result.errors[3].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[3].src_loc.GetLine() == 10u );
	U_TEST_ASSERT( build_result.errors[4].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[4].src_loc.GetLine() == 11u );
	U_TEST_ASSERT( build_result.errors[5].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[5].src_loc.GetLine() == 12u );
}

U_TEST(OperationNotSupportedForThisTypeTest1)
{
	// Indexation operators.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Bar(){}
		fn Foo()
		{
			var f32 variable= 0.0f32;
			var S s= zero_init;
			variable[ 42u32 ]; // Indexation of variable.
			s[ 45u32 ]; // Indexation of struct variable.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 2u );
	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[0].src_loc.GetLine() == 8u );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::OperationNotSupportedForThisType );
	U_TEST_ASSERT( build_result.errors[1].src_loc.GetLine() == 9u );
}

U_TEST(OperationNotSupportedForThisTypeTest2)
{
	// Member access operators.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			var f32 variable= 0.0f32;
			var [ u8, 16 ] s= zero_init;
			variable.m; // Member access of variable.
			s.size; // Member access of array.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 8u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest3)
{
	// Unary minus.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Bar(){}
		fn Foo()
		{
			var S s= zero_init;
			var [ u8, 16 ] a= zero_init;
			-s; // Unary minus for struct variable.
			-Bar; // Unary minus for of function.
			-a; // Unary minus for array.
			-false; // Unary minus for bool
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 11u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest4)
{
	// Increment and decrement.
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		struct S{}
		fn Foo( [ i32, 4 ] &mut arr, tup[ f32, bool ] &mut t, (fn()) &mut fn_ptr, E &mut e, S &mut s )
		{
			--arr;
			++t;
			--fn_ptr;
			++e;
			--s;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  6u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 10u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest5)
{
	// Bitwise not.
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		struct S{}
		fn Foo( [ i32, 4 ] &mut arr, tup[ f32, bool ] &mut t, (fn()) &mut fn_ptr, E &mut e, S &mut s )
		{
			~arr;
			~t;
			~fn_ptr;
			~e;
			~s;
			~0.35f;
			~'7';
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  6u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 11u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 12u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest6)
{
	// Logical not.
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		struct S{}
		fn Foo( [ i32, 4 ] &mut arr, tup[ f32, bool ] &mut t, (fn()) &mut fn_ptr, E &mut e, S &mut s )
		{
			!arr;
			!t;
			!fn_ptr;
			!e;
			!s;
			!0.35f;
			!'7';
			! 678;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  6u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 11u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 12u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 13u ) );
}

U_TEST(OperationNotSupportedForThisTypeTest7)
{
	// Shifts for unsupported types.
	static const char c_program_text[]=
	R"(
		enum E{ A, B, C }
		struct S{}
		fn Foo( [ i32, 4 ] &mut arr, tup[ f32, bool ] &mut t, (fn()) &mut fn_ptr, E &mut e, S &mut s )
		{
			arr << 1;
			t >> 2;
			3 << fn_ptr;
			787 >> e;
			0.25f << 5;
			135u << 3.5;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  6u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::OperationNotSupportedForThisType, 11u ) );
}

U_TEST(TypesMismatchTest0)
{
	// Expected 'bool' in 'if'.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( 42 )
			{
			}
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest1)
{
	// Expected 'bool' in 'while'.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( 0.25f32 )
			{
				break;
			}
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest2)
{
	// Unexpected type in assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			x= 3.1415926535f32;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(TypesMismatchTest3)
{
	// Unexpected type in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return 0.25f32;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest4)
{
	// Unexpected void in return.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest5)
{
	// Unexpected type in bindind to reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var i8 &x_ref= x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(TypesMismatchTest6)
{
	// Unexpected type in construction - "bool" to integer.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 i(false);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest7)
{
	// Unexpected type in construction - "bool" to float.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f32 f(true);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest8)
{
	// Unexpected type in construction - integer to "bool".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var bool b(58i16);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest9)
{
	// Unexpected type in construction - float to "bool".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var bool b( 0.5f64 );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TypesMismatchTest10)
{
	// Unexpected type in assignment - assign struct to integer.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var bool b= S();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(TypesMismatchTest11)
{
	// Unexpected type in reference-field initialization.
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		fn Foo()
		{
			var f32 x= 0.0f;
			var S s{ .x= x };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TypesMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(TypesMismatchTest12)
{
	// Unexpected type in array index.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo()
		{
			var [ i32, 16 ] arr= zero_init;
			var i32 mut x= 0;
			arr[ 0.25f ]; // f32
			arr[ 0.5 ]; // f64
			arr[ 'Z' ]; // char
			arr[ x ]; // non-constexpr signed integer
			arr[ S() ]; // struct type
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 11u ) );
}

U_TEST(TypesMismatchTest13)
{
	// Invalid types in lazy logical operator.
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo( S s, tup[] t, [ f32, 16 ] a )
		{
			false && s;
			s || true;
			t && false;
			true || t;
			a || false;
			false && a;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  5u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  6u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  7u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch,  9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::TypesMismatch, 10u ) );
}

U_TEST(NoMatchBinaryOperatorForGivenTypesTest0)
{
	// Add for array and int.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var [ i32, 4 ] y= zero_init;
			x + y;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(NoMatchBinaryOperatorForGivenTypesTest1)
{
	// Add for arrays.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var [ i32, 4 ] y= zero_init;
			x + y;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(NoMatchBinaryOperatorForGivenTypesTest2)
{
	// Bitwise operator for different types.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var [ i32, 4 ] y= zero_init;
			x ^ y;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoMatchBinaryOperatorForGivenTypes );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(ArraySizeIsNotInteger_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 5.0f32 ] x;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNotInteger );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ArraySizeIsNotInteger_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[i32] constexpr s= zero_init;
			var [ i32, s ] x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNotInteger );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ArraySizeIsNegative)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, -5 ] x;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArraySizeIsNegative );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( ExpectedVariableInArraySizeTest0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, f64 ] x; // Array size iz typename, but expected variable
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 4u ) );
}

U_TEST(BreakOutsideLoopTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			break;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::BreakOutsideLoop );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ContinueOutsideLoopTest)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			continue;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ContinueOutsideLoop );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(NameIsNotTypeNameTest)
{
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Foo()
		{
			var Bar i;
			return;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameIsNotTypeName );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(UnreachableCodeTest0)
{
	// Simple unreachable code.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			return;
			1 + 2;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::UnreachableCode, 5u ) );
}

U_TEST(UnreachableCodeTest1)
{
	// Unreachable code, when return is in inner block.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			{ return; }
			Bar();
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(UnreachableCodeTest2)
{
	// Unreachable code, when return is in if-else block.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( false ) { return; }
			else { return; }
			Bar();
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(UnreachableCodeTest3)
{
	// Should not generate unreachable code, when if-else block returns not in all cases.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( false ) { }
			else { return; }
			Bar();
			return;
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest4)
{
	// Should not generate unreachable code, when "if" block does not contains unconditional "else".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			if( true ) { return; }
			else if( false ) { return; }
			Bar();
			return;
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest5)
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				break;
				Bar();
			}
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST(UnreachableCodeTest6)
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				{ continue; }
				Bar();
			}
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST(UnreachableCodeTest7)
{
	// Unreachable code, when break/continue.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else { break; }
				Bar();
			}
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnreachableCode );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST(UnreachableCodeTest8)
{
	// Should not generate unreachable code, when break or continue is not in all if-branches.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else { }
				Bar();
			}
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(UnreachableCodeTest9)
{
	// Should not generate unreachable code, when "if" block does not contains unconditional "else".
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			while( true )
			{
				if( true ) { continue; } else if( false ) { break; }
				Bar();
			}
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest0)
{
	// No return in non-void function;
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest1)
{
	// Return not in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			if( true ) { return 0; }
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest2)
{
	// Return not in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			while( true ) { return 0; }
			if( false ) {} else { return 1; }
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NoReturnInFunctionReturningNonVoid );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest3)
{
	// Return exists in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			{
				if( true ) { return 42; }
			}
			Bar();
			return -1;
		}
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(NoReturnInFunctionReturningNonVoidTest4)
{
	// Return exists in all branches.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32
		{
			{
				if( true ) { return 42; }
				else { { return 666; } }
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(ExpectedReferenceValueTest0)
{
	// Assign to non-reference value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			1 + 2 = 42;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedReferenceValueTest2)
{
	// Assign to value.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 a, i32 b )
		{
			42 = b;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedReferenceValueTest3)
{
	// Assign to immutable value.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var f64 imut a= 3.1415926535f64;
			a = 0.0f64;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedReferenceValueTest4)
{
	// Assign to immutable argument.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 imut a )
		{
			a = -45;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedReferenceValueTest5)
{
	// Initialize reference using value-object.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 a= 42, b= 24;
			var i32 &x= a - b;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedReferenceValueTest6)
{
	// Using value in reference - function argument.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) {}
		fn Foo()
		{
			Bar(42);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedReferenceValueTest7)
{
	// Using value in reference - function return value.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 &
		{
			return 42;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedReferenceValueTest8)
{
	// Non "const reference" value used in compound assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 imut x= 2;
			x/= 5;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedReferenceValueTest9)
{
	// Non "const reference" value used in compound assignment.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			5/= 5;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedReferenceValue );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedVariableInAssignmentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Bar(i32 x){}
		fn Foo()
		{
			var i32 x= 0;
			Bar= x; // assign variable to function
			x= Bar; // assign function to variable
			C= x;   // assign variable to struct
			x= C;   // assign struct to variable
			C= Bar; // assign function to struct
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 11u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 12u ) );
}

U_TEST(ExpectedVariableInBinaryOperatorTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Bar(i32 x){}
		fn Foo()
		{
			var i32 x= 0;
			Bar + x;  // variable and functions set
			x / Bar;  // functions set and variable
			C * x;    // variable and struct
			x - C;    // struct and variable
			C == Bar; // functions set and struct
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 11u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 12u ) );
}

U_TEST(ExpectedVariableAsArgumentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar( i32 x ){}
		fn Bar(){}
		fn Foo()
		{
			var i32 x= 0;
			Bar( Bar ); // Functions set as argument
			Bar( C ); // struct as argument
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 9u ) );
}

U_TEST(ExpectedVariableInCompoundAssignmentTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Bar(i32 x){}
		fn Foo()
		{
			var i32 x= 0;
			Bar+= x;  // variable and functions set
			x/= Bar;  // functions set and variable
			C*= x;    // variable and struct
			x-= C;    // struct and variable
			C|= Bar; // functions set and struct
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 11u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 12u ) );
}

U_TEST(ExpectedVariableInIncrementOrDecrementTest0)
{
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Bar(){}
		fn Bar(i32 x){}
		namespace NS{}
		fn Foo()
		{
			++Bar; // functions set
			--C;   // class name
			++u32; // type name
			--NS;  // namespace
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 8u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 9u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 10u ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 11u ) );
}

U_TEST(ExpectedVariableInReferenceCastOperatorsTest0)
{
	static const char c_program_text[]=
	R"(
		namespace NS{}
		fn Foo()
		{
			unsafe{  cast_ref_unsafe</ i32 />( f32 );  }  // type name
			auto& x= cast_imut( NS );  // namespace
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 2u );

	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 5 ) );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ExpectedVariable, 6 ) );
}

U_TEST(CouldNotOverloadFunctionTest1)
{
	// Different are only mutability modifiers for value parameters.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 imut x ) {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(CouldNotOverloadFunctionTest2)
{
	// One parameter is value, other is const-reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(CouldNotOverloadFunctionTest3)
{
	// Const and nonconst reference-parameters are different.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x ) {}
		fn Foo( i32 &imut x ) {}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.empty() );
}

U_TEST(CouldNotSelectOverloadedFunction0)
{
	// Different actual args and args from functions set.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) {}
		fn Foo( f32 x ) {}
		fn Bar()
		{
			Foo( false );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(CouldNotSelectOverloadedFunction1)
{
	// Different actual args count and args from functions set.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) {}
		fn Foo( f32 x ) {}
		fn Bar()
		{
			Foo( 1, 2, 3, 4 );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotSelectOverloadedFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST(FunctionPrototypeDuplicationTest0)
{
	// Simple prototype duplication.
	static const char c_program_text[]=
	R"(
		fn Bar();
		fn Bar();
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST(FunctionPrototypeDuplicationTest1)
{
	// Functions with args of same type but different name is same.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y );
		fn Bar( i32 xx, f64 yy );
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionPrototypeDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST(FunctionBodyDuplicationTest0)
{
	// Simple body duplication.
	static const char c_program_text[]=
	R"(
		fn Bar(){}
		fn Bar(){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST(FunctionBodyDuplicationTest1)
{
	// Functions with args of same type but different name is same.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ){}
		fn Bar( i32 xx, f64 yy ){}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FunctionBodyDuplication );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest0)
{
	// Different return value type.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32;
		fn Bar( i32 x, f64 y ) : bool
		{
			return false;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest1)
{
	// Different return value mutability.
	static const char c_program_text[]=
	R"(
		fn Bar( i32& x, f64& y ) : i32 &imut;
		fn Bar( i32& x, f64& y ) : i32 &mut
		{
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest2)
{
	// Different reference modificator.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32&;
		fn Bar( i32 x, f64 y ) : i32
		{
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(ReturnValueDiffersFromPrototypeTest3)
{
	// Different mutability of returned reference-value.
	static const char c_program_text[]=
	R"(
		fn Bar( i32 x, f64 y ) : i32 &imut;
		fn Bar( i32 x, f64 y ) : i32 &mut
		{
			return 0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CouldNotOverloadFunction );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u || error.src_loc.GetLine() == 3u );
}

U_TEST(LineCounting_Test0)
{
	// \r\n count as signle line ending.
	static const char c_program_text[]= "auto Abc= 0;\r\nauto def= abc;";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 2u ) )
}

U_TEST(LineCounting_Test1)
{
	// \r and \n with symbols inbetween count as two line endings.
	static const char c_program_text[]= "auto Abc= 0;\r \nauto def= abc;";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 3u ) )
}

U_TEST(LineCounting_Test2)
{
	// Should count line endings inside multiline comments properly.
	static const char c_program_text[]= "auto Abc= 0;/* some comment \nwith \rline breaks\r\nkek*/auto def= abc;";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::NameNotFound, 4u ) )
}

} // namespace

} // namespace U
