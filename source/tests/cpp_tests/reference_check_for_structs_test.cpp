#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST( BasicReferenceInVariableCheck )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto &mut r1= x; // Accessing "x", which already have mutable reference inside "s".
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 7u ) );
}

U_TEST( DestructionOfVariableWithReferenceDestroysReference )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			{
				var S s{ .x= x };
			}
			auto &mut r1= x; // Ok, reference to "x" inside "s" already destroyed.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( LockVariableMultipleTimesInSameStruct_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			i32 &mut  y;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s{ .x= x, .y= x };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InnerReferenceMutabilityChanging, 11u ) );
}

U_TEST( LockVariableMultipleTimesInSameStruct_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			i32 &mut y;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s{ .x= x, .y= x };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( LockVariableMultipleTimesInSameStruct_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut  x;
			i32 &imut y;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s{ .x= x, .y= x };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::InnerReferenceMutabilityChanging, 11u ) );
}

U_TEST( LockVariableMultipleTimesInSameStruct_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			i32 &imut y;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s{ .x= x, .y= x };
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceFromArg_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo( S s't' ) : i32 &'t
		{
			return s.x; // Ok, allowed
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceFromArg_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo( S s't', i32 &'p i ) : i32 &'p
		{
			return s.x; // Error, does not allowed
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReturnReferenceFromArg_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			fn Foo( this, i32 &'p i ) : i32 &'p
			{
				return this.x; // Error, does not allowed
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 7u );
}

U_TEST( ReturnReferenceFromArg_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn Foo( this, i32 &'p i ) : i32 &'this
			{
				return this.x; // Ok, return this
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceFromArg_Test4 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn Foo( this'inner_this_shit', i32 &'p i ) : i32 &'inner_this_shit
			{
				return this.x; // Ok, return inner_this_shit
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceFromArg_Test5 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
		}

		fn Foo( S s ) : i32 &mut // Returning reference tag not specified, assume, that function can return any reference-arg, but not any reference from arg.
		{
			return s.x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( ReturnReferenceFromArg_Test6 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		// Specify impossible return references combination - return reference to first arg with inner reference to second arg.
		fn Foo( S &'x a'x_inner', S &'y b'y_inner' ) : S'y_inner' &'x
		{
			return a;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 6u );
}

U_TEST( ReturnReferenceFromArg_Test7 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		// Ok - return reference to one of args (with specifying reference tags).
		fn Foo( bool cond, S &'x a'x_inner', S &'x b'x_inner' ) : S'x_inner' &'x
		{
			if( cond ) { return a; }
			else { return b; }
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceFromArg_Test8 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		// Specify impossible return references combination - return reference to first arg with inner reference to second arg.
		// This is not a big problem, since there is not possible to write correct implementation of this function without unsafe hacks.
		fn Foo( S &'x a'x_inner', S &'y b'y_inner' ) : S'y_inner' &'x;
		fn Bar()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			var S s{ .x= c };
			{
				var S mut s_a{ .x= a };
				var S mut s_b{ .x= b };
				var S &impossible_ref= Foo( s_a, s_b );
				auto& b_ref= s_b.x; // Creating here reference to "s_b.x", which is wrongly linked with inner reference node of "impossible_ref".
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ReturnReferenceFromArg_Test9 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo( S &'x a'x_inner', S &'y b'y_inner' ) : S'x_inner' &'x;
		fn Bar()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			var S s{ .x= c };
			{
				var S mut s_a{ .x= a };
				var S mut s_b{ .x= b };
				var S &s_a_ref= Foo( s_a, s_b );
				auto& b_ref= s_b.x; // Ok, "s_b_ref" points to both node itself and node inner reference of "s_a", but not "s_b".
			}
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReturnReferenceToLocalVariableInsideStruct )
{
	static const char c_program_text[]=
	R"(
		struct S { i32& x; }

		fn Foo() : S
		{
			var i32 x= 0;
			var S s{ .x= x };
			return s;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReturningUnallowedReference, 8u ) );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::DestroyedVariableStillHaveReferences, 8u ) );
}

U_TEST( GetReturnedReferencePassedThroughArgument_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Foo( S s't' ) : i32 &'t mut
		{
			return s.x;
		}

		fn Baz()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto &mut r0= Foo( s );
			++s.x; // Error, reference to 'x' inside 's' is not terminal.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 13u );
}

U_TEST( GetReturnedReferencePassedThroughArgument_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S { i32 &mut x; }
		fn Baz( i32 &'r mut i, S s't' ) : i32 &'r mut
		{
			return i;
		}

		fn Foo() : i32
		{
			var i32 mut x= 0;
			var i32 mut y= 1;
			{
				var S s{ .x= x };
				auto &mut r0= Baz( y, s ); // ok, "r0" refers to "y", but not x."
				r0= 4456823;
			}
			return y;
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );
	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	llvm::GenericValue result_value= engine->runFunction(function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>( 4456823 ) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ReturnStructWithReferenceFromFunction_Test0 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct MutRef{ T &mut r; }

		fn ToRef( i32 &'r mut x ) : MutRef</ i32 />'r'
		{
			var MutRef</ i32 /> r{ .r= x };
			return r;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r0= x;
			auto &mut r1= ToRef( r0 ).r;
			--r0; // Error, reference to 'x' inside 's' is not terminal.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ReferenceProtectionError, 15u ) );
}

U_TEST( ReturnStructWithReferenceFromFunction_Test1 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct MutRef{ T &mut r; }

		fn ToRef( i32 &'r mut x ) : MutRef</ i32 />'r' // References now implicitly tagged
		{
			var MutRef</ i32 /> r{ .r= x };
			return r;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r0= x;
			auto &mut r1= ToRef( x ).r; // Error, reference, inside struct, refers to "x", but there is reference "r0" on stack.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ReturnStructWithReferenceFromFunction_Test2 )
{
	static const char c_program_text[]=
	R"(
		template</ type T /> struct ImutRef{ T &imut r; }

		fn ToRef( i32 &'r mut x, i32 &'f imut y ) : ImutRef</ i32 />'f' // References now implicitly tagged. Returning only one reference.
		{
			x= y;
			var ImutRef</ i32 /> r{ .r= y };
			return r;
		}

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			auto &mut r0= x;
			auto &imut r1= ToRef( r0, y ).r; // Ok, reference, inside struct, refers to "y".
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( TwoLevelsOfIndirection_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct A{ i32 &mut x; }
		struct B{ A   &imut x; }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceFieldOfTypeWithReferencesInside );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( TwoLevelsOfIndirection_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct A{ i32 &mut x; }
		struct B{ A   &imut x; }
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceFieldOfTypeWithReferencesInside );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( ReferencePollutionTest0 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &mut x; }

		fn Foo( S &mut s'x', i32 &'y mut i ) ' x <- y '
		{}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferencePollutionTest1 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }

		// Function takes mutable argumebnt, but links it with other argument as immutable.
		fn Foo( S &mut s'x', i32 &'y mut i ) ' x <- y '
		{}

		fn Foo()
		{
			var i32 mut y= 0, mut x= 0;
			var S mut s { .x= x };
			Foo( s, y ); // Now "s" contains immutable reference to "y", even if we mass into function mutable reference.
			auto &imut y_ref= y; // Ok, can take second immutable refence.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ReferencePollutionTest2_LinkAsImmutableIfAllLinkedVariablesAreMutable )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		fn Baz( S &mut s_dst'x', S &imut s_src'y' ) ' x <- y '
		{}

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s0{ .x= x };
			{
				var S mut s1{ .x= y };
				Baz( s0, s1 ); // Now s0 contains immutable reference to "y", even if function reference pollution is mutable - because s1 contains only immutable reference.
			}
			auto &mut y_mut_ref= y; // Error, s0 contains reference to y.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ReferencePollutionTest3_LinkAsImmutableIfAllLinkedVariablesAreMutable )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		fn Baz( S &mut s_dst'x', i32 &'y imut i ) ' x <- y '
		{}

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			Baz( s, y ); // Now "s" contains immutable reference to "y".
			auto &mut y_mut_ref= y; // Error, "s" contains reference to "y".
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( ReferencePollutionTest4_LinkAsImmutableIfAllLinkedVariablesAreMutable )
{
	static const char c_program_text[]=
	R"(
		struct MutRefTag{ i32& mut x; }
		struct S{ [ MutRefTag, 0 ] ref_tag; }
		fn Baz( S &mut s_dst'x', i32 &'y imut i ) ' x <- y '
		{}

		fn Foo()
		{
			var i32 mut y= 0;
			var S mut s;
			Baz( s, y ); // Now "s" contains immutable reference to "y", even if function reference pollution is mutable - because "y" passed into function as immutable.
			auto& y_ref= y; // Ok, creating another immutable reference.
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ConstructorLinksPassedReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this't', i32 &'p mut in_x ) ' t <- p '
			( x(in_x) )
			{}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S s( x ); // Constructor links passed reference with itself.
			++x; // Error, "x" have mutable references.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ConstructorLinksPassedReference_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this't', i32 &'p in_x ) ' t <- p '
			( x(in_x) )
			{}
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= S(x).x; // Constructed temp temp variable refers to "x", then, reference to "x" taked and saved.
			++x; // Error, "x" have immutable reference.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ConvertedVariableCanLostInnerReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			fn constructor( A &imut other )= default;
		}
		class B : A
		{
			i32 &mut x;
			fn constructor( this't', i32 &'p mut in_x ) ' t <- p '
			( x(in_x) )
			{}
			fn constructor( B &imut other )= default;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a= B(x); // Save refernce to 'x' in temp variable, but in initialization we loss this reference.
			++x; // Ok,
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( ConvertedVariableCanLostInnerReference_Test1 )
{
	static const char c_program_text[]=
	R"(
		class A polymorph
		{
			fn constructor( A &imut other )= default;
			op=( mut this, A &imut other )= default;
		}
		class B : A
		{
			i32 &mut x;
			fn constructor( this't', i32 &'p mut in_x ) ' t <- p '
			( x(in_x) )
			{}
			fn constructor( B &imut other )= default;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var A mut a;
			{
				var B b(x); // Save reference to 'x' in 'b'.
				a= b; // This assignment must not transfer reference fro 'b' to 'a'.
			}
			++x; // Ok,
		}
	)";

	BuildProgram( c_program_text );
}

U_TEST( AutoVariableContainsCopyOfReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this'st', i32 &'r in_x ) ' st <- r '
			( x= in_x )
			{}
		}

		fn Foo()
		{
			var i32 mut y= 0;
			auto s= S(y);
			++y; // Error, 's' have reference to 'y'.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( ExpressionInitializedVariableContainsCopyOfReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this'st', i32 &'r in_x ) ' st <- r '
			( x= in_x )
			{}
		}

		fn Foo()
		{
			var i32 mut y= 0;
			var S s= S(y);
			++y; // Error, 's' have reference to 'y'.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( CopyAssignmentOperator_PollutionTest )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			op=( mut this, S &imut other ) // Have implicit reference pollution
			{} // Actually does nothing.
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var S mut s{ .x= x };
			{
				var i32 mut y= 0;
				var S s1{ .x= y };
				s= s1; // Now, 's' contains reverences to 'x' and 'y'.
			}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 17u );
}

U_TEST( ReferencePollutionErrorsTest_SelfReferencePollution )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &mut x; }
		fn Foo( S &mut s'x' ) ' x <- x '
		{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::SelfReferencePollution );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( ReferencePollutionErrorsTest_ArgReferencePollution )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &mut x; }
		fn Foo( S &mut s'x', i32 &'y mut r ) ' y <- x '
		{}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );
	U_TEST_ASSERT( HaveError( build_result.errors, CodeBuilderErrorCode::ArgReferencePollution, 3u ) );
}

U_TEST( ReferencePollutionErrorsTest_UnallowedReferencePollution_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		fn FakePollution( S &mut s'x', i32 &'y i ) ' x <- y ' // reference pollution allowed in signature, but actually not happens.
		{}

		fn Foo( S &mut s, i32 & r )
		{
			FakePollution( s, r );
		} // Error, pollution of "s" with "r", which is not allowed.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST( ReferencePollutionErrorsTest_UnallowedReferencePollution_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		fn FakePollution( S &mut s'x', i32 &'y i ) ' x <- y ' // reference pollution allowed in signature, but actually not happens.
		{}

		fn Foo( S &mut s, i32 & r )
		{
			auto inner_int= 0;
			FakePollution( s, inner_int );
		} // Error, pollution of "s" with inner variable, which is not allowed.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( ReferencePollutionErrorsTest_UnallowedReferencePollution_Test2 )
{
	// "this" inner variables pollution.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn FakePollution( mut this'x', i32 &'y i ) ' x <- y '// reference pollution allowed in signature, but actually not happens.
			{}
		}

		fn Foo( S &mut s, i32 & r )
		{
			s.FakePollution(r);
		} // Error, pollution of "s" with "r", which is not allowed.
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( error.src_loc.GetLine() == 12u );
}

U_TEST( ReferencePollutionErrorsTest_UnallowedReferencePollution_Test3 )
{
	// "this" inner variables pollution in constructor.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this, i32 &imut in_x )( x= in_x ) {}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferencePollutionErrorsTest_ExplicitReferencePollutionForCopyConstructor )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32& x;
			fn constructor( mut this'x', S & other'y' ) ' x <- y '
			( x(other.x) )
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitReferencePollutionForCopyConstructor );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}


U_TEST( ReferencePollutionErrorsTest_ExplicitReferencePollutionForCopyAssignmentOperator )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32& x;
			op=( mut this'x', S & other'y' ) ' x <- y '
			{}
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitReferencePollutionForCopyAssignmentOperator );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( ReferencePollutionErrorsTest_ExplicitReferencePollutionForEqualityCompareOperator )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32& x;
			op==( S& a'x', S& b'y') ' x <- y ' : bool;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitReferencePollutionForEqualityCompareOperator );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST( TryGrabReferenceToTempVariable_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
		}
		struct R
		{
			i32& r;
		}
		fn Foo()
		{
			var R r{ .r= S(42).x }; // Grab reference to temp variable in expression-initialization of reference field.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( TryGrabReferenceToTempVariable_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
		}
		struct R
		{
			i32& r;
		}
		fn Foo()
		{
			var R r{ .r( S(42).x ) }; // Grab reference to temp variable in constructor-initialization of reference field.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 14u );
}

U_TEST( TryGrabReferenceToTempVariable_Test2 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
			( x= in_x ) {}
		}
		struct R
		{
			i32& r;
			fn constructor( this'a', i32&'b in_r ) ' a <- b '
			( r= in_r ) {}
		}
		fn Foo()
		{
			var R r( S(42).x ); // Grab reference to temp variable in constructor initializer.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 16u );
}

U_TEST( TryGrabReferenceToTempVariable_Test3 )
{
	static const char c_program_text[]=
	R"(
		struct R
		{
			i32& r;
			fn constructor( this'a', i32&'b in_r ) ' a <- b '
			( r= in_r ) {}
		}
		fn Foo()
		{
			var R r( 42 ); // Grab reference to temp variable in constructor initializer.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( TryGrabReferenceToTempVariable_Test4 )
{
	static const char c_program_text[]=
	R"(
		struct R
		{
			i32& r;
			fn constructor( this'a', i32&'b in_r ) ' a <- b '
			( r= in_r ) {}
		}
		struct T{ R r; }
		fn Foo()
		{
			var T t{ .r( 42 ) }; // Grab reference to temp variable in constructor initializer of struct member.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 11u );
}

U_TEST( TryGrabReferenceToTempVariable_Test5 )
{
	static const char c_program_text[]=
	R"(
		struct R
		{
			i32& r;
			fn constructor( this'a', i32&'b in_r ) ' a <- b '
			( r= in_r ) {}
		}
		fn Foo()
		{
			var [ R, 1 ] r[ ( 42 ) ]; // Grab reference to temp variable in constructor initializer of array member.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( NameNotFound_ForReferenceTags_Test0 )
{
	// Unknown tag for return reference.
	static const char c_program_text[]=
	R"(
		fn Foo( i32& x ) : i32&'a;  // Error, tag 'a' not found
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( NameNotFound_ForReferenceTags_Test1 )
{
	// Unknown tag for return value inner reference.
	static const char c_program_text[]=
	R"(
		struct S{ i32& r; }
		fn Foo( i32& x ) : S'a';  // Error, tag 'a' not found
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

U_TEST( NameNotFound_ForReferenceTags_Test2 )
{
	// Unknown tag in reference pollution list.
	static const char c_program_text[]=
	R"(
		struct S{ i32& r; }
		fn Foo( S &mut s'fff', i32& x ) ' fff <- ttt ';  // Error, tag 'ttt' not found
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();
	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 3u );
}

} // namespace

} // namespace U
