#include "tests.hpp"


namespace U
{

U_TEST( ReferenceCheckTest_MultipleMutableReferencesOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &mut r0= x;
			auto &mut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MutableReferenceAfterImmutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto &imut r0= x;
			auto & mut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_ImmutableReferenceAfterMutableReferenceOnStack )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			auto & mut r0= x;
			auto &imut r1= x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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
			var i32 x= 0;
			Bar( x, x );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_MutableAndImmutableReferencesPassedToFunction )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 x= 0;
			Bar( x, x );
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
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
			var i32 x= 0;
			auto &imut r0= Bar( x ); // r0 refers to x
			auto & mut r1= x; // r1 refers to x too. Mutable reference after immutable forbidden.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ReferenceCheckTest_FunctionWithSingleArgumentReturnsReferenceToInputVariable_1 )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) : i32 &mut { return x; }
		fn Baz( i32 &mut x, i32 &mut y ) {}
		fn Foo()
		{
			var i32 x= 0;
			Baz( Bar(x), Bar(x) ); // Result of two Bar calls produces two mutable references to x. Passing this references into Baz is forbidden.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ReferenceCheckTest_StructMemberRefersToStruct_0 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			var S s= zero_init;
			var i32 &mut r0= s.x; // r0 refers to s
			var S &imut r1= s; // r1 refers to s too
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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
			var S s= zero_init;
			var i32 &mut r0= s.GetThis().x; // r0 refers to s
			var S &imut r1= s.GetThis(); // r1 refers to s too
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 12u );
}


U_TEST( ReferenceCheckTest_ArrayMemberRefersToArray_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 4 ] arr= zero_init;
			auto &mut r0= arr[1u];
			auto &mut r1= arr[2u];
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_ArrayMemberRefersToArray_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 4 ] arr= zero_init;
			auto &imut r0= arr[1u];
			auto &mut r1= arr;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_DestroyedReferenceDoesNotRefersToVariable )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ) {}
		fn Foo()
		{
			var i32 x= 0;
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
			var i32 x= 0;
			auto &mut r0= x;
			{
				auto &mut r1= x; // Error. Here visible two mutable references to x - r1 and r0.
			}
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 8u );
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
			var i32 x= 0, y= 0;
			var i32 &imut max= Max( x, y ); // max refers to both x and y.
			var i32 &mut x_ref= x; // Taking second reference to x.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 11u );
}

U_TEST( ReferenceCheckTest_passMutableReferenceToFunctionWhenMutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			var i32 x= 0;
			var i32 &mut r= x;
			Bar(x); // Forbidden, x already have mutable reference.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ReferenceCheckTest_PassMutableReferenceToFunctionWhenImmutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &mut x ){}
		fn Foo()
		{
			var i32 x= 0;
			var i32 &imut r= x;
			Bar(x); // Forbidden, x already have immutable reference.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ReferenceCheckTest_PassImmutableReferenceToFunctionWhenMutableReferenceOnStackExists )
{
	static const char c_program_text[]=
	R"(
		fn Bar( i32 &imut x ){}
		fn Foo()
		{
			var i32 x= 0;
			var i32 &mut r= x;
			Bar(x); // Forbidden, x already have mutable reference.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	//U_TEST_ASSERT( error.file_pos.line == 7u ); // TODO - check this
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	//U_TEST_ASSERT( error.file_pos.line == 7u ); // TODO - check this
}

U_TEST( ReferenceCheckTest_ReturnReferenceToLocalVariable_2 )
{
	// Conditional return of reference to member of stack variable, pussed through function.
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	//U_TEST_ASSERT( error.file_pos.line == 7u ); // TODO - check this
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

U_TEST( ReferenceCheckTest_ReturnReferenceToValueArgument_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x ) : i32 &imut
		{
			return x;
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	//U_TEST_ASSERT( error.file_pos.line == 7u ); // TODO - check this
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	//U_TEST_ASSERT( error.file_pos.line == 7u ); // TODO - check this
}

U_TEST( ReferenceCheckTest_ReferenceShouldLockVariableAfterConditionalReturn )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var i32 &mut r0= x;
			if( false ){ return; }
			var i32 &mut r1= x; // "x" reference counters should be unchanged after return.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var i32 &mut r= x;
			x= 24; // Error, "x" have mutable reference.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

U_TEST( ReferenceCheckTest_AssignmentForReferencedVariable_1 )
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x= 0;
			var i32 &imut r= x;
			x= 24; // Error, "x" have immutable reference.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 6u );
}

} // namespace U
