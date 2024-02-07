#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( ReferenceCheckTest_MultipleMutableReferencesOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r0= x;
			auto &mut r1= x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_MutableReferenceAfterImmutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r0= x;
			auto & mut r1= x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_ImmutableReferenceAfterMutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto & mut r0= x;
			auto &imut r1= x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_MultipleImmutableReferencesShouldBeOk )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &imut r0= x;
			auto &imut r1= x;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_MultipleMutableReferencesPassedToFunction )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Bar( x, x );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_MutableAndImmutableReferencesPassedToFunction )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Bar( x, x );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_MultipleImmutableReferencesPassedToFunctionShouldBeOk )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x, i32 &imut y ) {}
		fn ToImut( i32 &imut x ) : i32 &imut { return x; }
		fn Foo()
		{
			var i32 x= 0;
			Bar( ToImut(x), ToImut(x) );
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_FunctionWithSingleArgumentReturnsReferenceToInputVariable_0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ) : i32 &imut { return x; }
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r0= Bar( x ); // r0 refers to x
			auto & mut r1= x; // r1 refers to x too. Mutable reference after immutable forbidden.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_FunctionWithSingleArgumentReturnsReferenceToInputVariable_1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) : i32 &mut { return x; }
		fn Baz( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Baz( Bar(x), Bar(x) ); // Result of two Bar calls produces two mutable references to x. Passing this references into Baz is forbidden.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( ReferenceCheckTest_StructMemberRefersToStruct_0 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			var S mut s= zero_init;
			var i32 &mut r0= s.x; // r0 refers to s
			var S &imut r1= s; // r1 refers to s too
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( ReferenceCheckTest_StructMemberRefersToStruct_1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn GetThis(  mut this ) : S & mut { return this; }
			fn GetThis( imut this ) : S &imut { return this; }
		}
		fn Foo()
		{
			var S mut s= zero_init;
			var i32 &mut r0= s.GetThis().x; // r0 refers to s
			var S &imut r1= s.GetThis(); // r1 refers to s too
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 12u ) );
}

U_TEST( ReferenceCheckTest_ArrayMemberRefersToArray_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			auto &mut r0= arr[1u];
			auto &mut r1= arr[2u];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_ArrayMemberRefersToArray_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			auto &imut r0= arr[1u];
			auto &mut r1= arr;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_DestroyedReferenceDoesNotRefersToVariable )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			{
				auto &mut r0= x;
			}
			auto &mut r1= x; // All ok - r0 already destroyed and r1 is only mutable reference to x.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_ReferenceInInnerScopeInteractsWithReferenceInOuterScope )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r0= x;
			{
				auto &mut r1= x; // Error. Here visible two mutable references to x - r1 and r0.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 8u ) );
}

U_TEST( ReferenceCheckTest_ReferenceCanReferToMultipleVariables )
{
	static const char c_program_text[]=
	R"(
		fn Max( i32 &imut a, i32 &imut b ) : i32 &imut
		{
			if( a > b ) { return a; }
			return b;
		}
		fn Foo()
		{
			var i32 mut x= 0, y= 0;
			var i32 &imut max= Max( x, y ); // max refers to both x and y.
			var i32 &mut x_ref= x; // Taking second reference to x.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( ReferenceCheckTest_PassMutableReferenceToFunctionWhenMutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r= x;
			Bar(x); // Forbidden, x already have mutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_PassMutableReferenceToFunctionWhenImmutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &imut r= x;
			Bar(x); // Forbidden, x already have immutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_PassImmutableReferenceToFunctionWhenMutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ){}
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r= x;
			Bar(x); // Forbidden, x already have mutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_0 )
{
	// Simple return of reference to stack variable.
	static const char c_program_text[]=
	R"(
		fn Foo() : i32 &imut
		{
			var i32 x= 0;
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_1 )
{
	// Conditional return of reference to stack variable.
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &imut in ) : i32 &imut
		{
			var i32 x= 0;
			if( in != 0 ){ return x; }
			return in;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_2 )
{
	// Conditional return of reference to member of stack variable, passed through function.
	static const char c_program_text[]=
	R"(
		struct S{ [ i32, 2 ] x; }
		fn Pass( S   &imut s ) : S   &imut { return s; }
		fn Pass( i32 &imut x ) : i32 &imut { return x; }
		fn Foo( i32 &imut in ) : i32 &imut
		{
			var S s= zero_init;
			while( in == 0 )
			{
				return Pass(Pass(s).x[0u]);
			}
			return in;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_3 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut in ) : i32 &imut
		{
			if( in == 42 ) { return in; }
			auto &mut r= in; // Should take this reference.
			return r;
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_4 )
{
	// Return of reference to stack variable, passed inside a struct.
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		fn GetS() : S;
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S &mut x, i32 & y ) @(pollution);
		fn Foo() : S
		{
			var S mut s= GetS();
			var i32 x= 0;
			DoPollution( s, x );
			return move(s);
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::DestroyedVariableStillHasReferences, 11u ) );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToValueArgument_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : i32 &imut
		{
			return x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST( ReferenceCheckTest_ReturnReferenceToValueArgument_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : i32 &imut
		{
			auto &ref= x;
			return ref;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_AssignToReferenceTemporaryVariable_0 )
{
	static const char c_program_text[]=
	R"(
		fn PassRef( i32 &imut x ) : i32 &imut { return x; }
		fn Foo()
		{
			auto &imut r= PassRef( 42 ); // r referes here to temporary variable "42".
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_AssignToReferenceTemporaryVariable_1 )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		fn PassRef( S &imut x ) : S &imut { return x; }
		fn Foo()
		{
			auto &imut r= PassRef( S() ); // r referes here to temporary variable of type S.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_AssignToReferenceTemporaryVariable_2 )
{
	static const char c_program_text[]=
	R"(
		fn PassRef( bool &imut x ) : bool &imut { return x; }
		fn Foo()
		{
			var bool &imut r= PassRef( false ); // r referes here to temporary variable of bool type.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHasReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_AssignToReferenceTemporaryVariable_3 )
{
	static const char c_program_text[]=
	R"(
		fn PassRef( bool &imut x ) : bool &imut { return x; }
		fn Foo()
		{
			var bool &imut r= PassRef( true && false ); // r referes here to temporary variable of bool type.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::DestroyedVariableStillHasReferences, 5u ) );
}

U_TEST( ReferenceCheckTest_ReferenceShouldLockVariableAfterConditionalReturn )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r0= x;
			if( false ){ return; }
			var i32 &mut r1= x; // "x" reference counters should be unchanged after return.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r= x;
			x= 24; // Error, "x" have mutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &imut r= x;
			x= 24; // Error, "x" have immutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r= x;
			r= 24; // Ok, assign value to "x", using single mutable reference.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_3 )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 x;
			op=( mut this, Box &imut other )
			{
				this.x= other.x;
			}
		}
		fn Foo()
		{
			var Box mut x= zero_init;
			x= x; // Self-assignment, should produce error.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 13u );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_4 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			x= i32(x); // Self-assignment, using deref, should be ok.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_5 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0;
			r0= 24; // Error, "r0" have child reference - "r1".
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_6 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[i32] mut x[0], y[1];
			auto& r= x;
			x= y; // Assign value to tuple, that have references.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_Increment_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= x;
			++x; // Error, x have immutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_Increment_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r= x;
			++x; // Error, x have mmutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_AdditiveAssignment_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r= x;
			x+= 1; // Error, x have mmutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_AdditiveAssignment_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= x;
			x-= 1; // Error, x have immutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_AdditiveAssignment_2 )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 b;
			op/=( mut this, Box &imut other )
			{
				this.b /= other.b;
			}
		}
		fn Foo()
		{
			var Box mut x{ .b= 541 };
			x/= x; // Error, pass into operator both mutable and immutable references.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 13u );
}

U_TEST( ReferenceCheckTest_ShouldConvertReferenceInFunctionCall_0 )
{
	static const char c_program_text[]=
	R"(
		fn Add( i32 x, i32 y ) : i32 { return x + y; }
		fn Foo()
		{
			var i32 mut x= 0;
			Add( x, x ); // We take here mutable references to x, but convert it to value in function call, so, this is not error.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_ShouldConvertReferenceInFunctionCall_1 )
{
	static const char c_program_text[]=
	R"(
		fn Add( i32 &imut x, i32 &imut y ) : i32 { return x + y; }
		fn Foo()
		{
			var i32 mut x= 0;
			Add( x, x ); // We take here mutable references to x, but convert it to immatable references in function call, so, this is not error.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_ShouldConvertReferenceInFunctionCall_2 )
{
	static const char c_program_text[]=
	R"(
		fn Assign( i32 x, i32 &mut y ) { y= x; }
		fn Foo()
		{
			var i32 mut x= 0;
			Assign( x, x ); // We take here mutable references to x, but convert first reference to value and pass to function only second reference.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferenceCheckTest_TryPassTwoMutableReferencesIntoFunction_0 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Bar( x, x ); // Take mutable reference, then, take mutable reference again
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 6u ) );
}

U_TEST( ReferenceCheckTest_TryPassTwoMutableReferencesIntoFunction_1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r= x;
			Bar( r, r ); // Use mutable reference from stack, then, use it again.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_TryPassTwoMutableReferencesIntoFunction_2 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r= x;
			Bar( r, x ); // Use mutable reference from stack, then, use variable itself.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( ReferenceCheckTest_TryUseVariableWhenReferenceInFunctionCallExists_0 )
{
	static const char c_program_text[]=
	R"(
		fn Deref( i32 &imut x ) : i32 { return x; }
		fn Do( i32 &mut x, i32 y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Do(
				x,
				Deref(x) ); // Pass immutable reference into function "Deref" when mutable reference for function call "Do" saved.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( ReferenceCheckTest_TryUseVariableWhenReferenceInFunctionCallExists_1 )
{
	static const char c_program_text[]=
	R"(
		fn Mutate( i32 &mut x ) : i32 { return x; }
		fn Do( i32 &imut x, i32 y ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			Do(
				x,
				Mutate(x) ); // Pass mutable reference into function "Deref" when immutable reference for function call "Do" saved.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( ReferenceCheckTest_TryModifyArrayInIndexing_Test0 )
{
	static const char c_program_text[]=
	R"(
		fn Modify( [ i32, 4 ] &mut arr ) : u32 { return 1u; }
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			auto el= arr[ Modify( arr ) ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_TryModifyArrayInIndexing_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> fn ToImut( T& t ) : T& { return t; }
		fn Modify( [ i32, 4 ] &mut arr ) : u32 { return 1u; }
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			auto el= ToImut(arr)[ Modify( arr ) ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_TryModifyArrayInIndexing_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S{ [ i32, 4 ] arr; }
		fn Modify( S &mut s ) : u32 { return 0u; }
		fn Foo()
		{
			var S mut s= zero_init;
			auto el= s.arr[ Modify( s ) ];   // Error, we can not modify 's' in array index calcualtion, because we lock array as part of 's'.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReferenceCheckTest_DeltaOneOperatorsModifyValue_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= x;
			++x; // Error, x have references
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReferenceCheckTest_DeltaOneOperatorsModifyValue_1 )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 b;
			op++( mut this )
			{
				++b;
			}
		}
		fn Foo()
		{
			var Box mut b= zero_init;
			auto &imut r= b;
			++b; // Error, x have references
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ReferenceCheckTest_BinaryOperatorsModifyValue )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 b;
			op+( mut this, Box &mut other ) // Correct, but useless declaration of operator
			{
				b+= other.b;
			}
		}
		fn Foo()
		{
			var Box mut b= zero_init;
			b + b;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HasError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 13u ) );
}

U_TEST( ReferenceCheckTest_AssignmentOperatorsModifyValue )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 b;
			op=( mut this, Box &mut other ) // Correct, but useless declaration of operator
			{
				b= other.b;
			}
		}
		fn Foo()
		{
			var Box mut b= zero_init;
			b= b;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 13u );
}

U_TEST( ReferenceCheckTest_AdditiveAssignmentOperatorsModifyValue )
{
	static const char c_program_text[]=
	R"(
		struct Box
		{
			i32 b;
			op+=( mut this, Box &mut other ) // Correct, but useless declaration of operator
			{
				b+= other.b;
			}
		}
		fn Foo()
		{
			var Box mut b= zero_init;
			b+= b;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 13u );
}

U_TEST( ReferenceCheckTest_TryChangeArgs0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x )
		{
			auto &imut r= x;
			x+= 42; // Can not mutate - have immutable references.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_TryChangeArgs1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x )
		{
			auto &mut r= x;
			x+= 42; // Can not access arg - have mutable references.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_TryChangeArgs2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &mut x )
		{
			auto &mut r0= x;
			++x; // Error, 'x' have mutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferenceCheckTest_MutableReferenceResultPointsOnlyToMutableArgs_Test0 )
{
	static const char c_program_text[]=
	R"(
		// Auto reference mapping is generated - result mutable reference points only to mutable args.
		fn Bar( i32 &mut x, i32 &imut y ) : i32 &mut;
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			auto &mut r= Bar( x, y );
			++y; // Can modifiy it, because "r" points only to "x".
		}
	)";

	BuildProgram( c_program_text );
}

} // namespace

} // namespace U
