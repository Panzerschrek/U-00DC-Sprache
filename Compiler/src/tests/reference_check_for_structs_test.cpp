#include "tests.hpp"

namespace U
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 11u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 11u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 11u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.file_pos.line == 7u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReturningUnallowedReference );
	U_TEST_ASSERT( error.file_pos.line == 9u );
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
			auto &mut r0= Foo( s ); // Error, r0 contains second mutable reference
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 12u );
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

		fn ToRef( i32 &mut x ) : MutRef</ i32 />
		{
			var MutRef</ i32 /> r{ .r= x };
			return r;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut r0= x;
			auto &mut r1= ToRef( r0 ).r; // Error, reference, inside struct, refers to "x", but there is reference "r0" on stack.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 14u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 14u );
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

		fn Baz( i32 &mut x, i32 &mut y ){}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .x= a };

			Baz( a.x, b.x.x ); // Error, both argument references refers to "x".
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 13u );
}

U_TEST( TwoLevelsOfIndirection_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct A{ i32 &mut x; }
		struct B{ A   &imut x; }

		fn Extract( B & b'x' ) : i32 &'x mut
		{
			return b.x.x;
		}
		fn Baz( i32 &mut x, i32 &mut y ){}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .x= a };

			Baz( Extract(b), a.x ); // Error, both argument references refers to "x".
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 17u );
}

U_TEST( ThreeLevelsOfIndirection_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct A{ i32 &mut x; }
		struct B{ A   &imut x; }
		struct C{ B   &imut x; }

		fn Extract( C & c'x' ) : i32 &'x mut
		{
			return c.x.x.x;
		}
		fn Baz( i32 &mut x, i32 &mut y ){}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .x= a };
			var C c{ .x= b };

			Baz( a.x, Extract(c) ); // Error, both argument references refers to "x".
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 19u );
}

U_TEST( ThreeLevelsOfIndirection_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct A{ i32 &mut x; }
		struct B{ A   &imut x; }
		struct C{ B   &imut x; }

		fn Extract( C & c'x' ) : i32 &'x mut
		{
			return c.x.x.x;
		}

		fn Foo()
		{
			var i32 mut x= 0;
			var A a{ .x= x };
			var B b{ .x= a };
			var C c{ .x= b };

			auto &imut ref= Extract(c); // error, 'ref' contains reference to 'x', while mutable reference inside 'a' exists.
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 18u );
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
		fn Foo( S &mut s'x', i32 &'y mut i ) ' x <- imut y '
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
		fn Baz( S &mut s_dst'x', S &imut s_src'y' ) ' x <- mut y '
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 14u );
}

U_TEST( ReferencePollutionTest3_LinkAsImmutableIfAllLinkedVariablesAreMutable )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		fn Baz( S &mut s_dst'x', i32 &'y imut i ) ' x <- mut y '
		{}

		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			Baz( s, y ); // Now "s" contains immutable reference to "y", even if function reference pollution is mutable - because "y" passed into function as immutable.
			auto &mut y_mut_ref= y; // Error, "s" contains reference to "y".
		}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 11u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::AccessingVariableThatHaveMutableReference );
	U_TEST_ASSERT( error.file_pos.line == 14u );
}

U_TEST( ConstructorLinksPassedReference_Test1 )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &imut x;
			fn constructor( this't', i32 &'p in_x ) ' t <- imut p '
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferenceProtectionError );
	U_TEST_ASSERT( error.file_pos.line == 14u );
}

U_TEST( CopyAssignmentOperator_PollutionTest )
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 &mut x;
			op=( this'x', S &imut other'y' ) ' x <- mut y '
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( error.file_pos.line == 17u );
}

U_TEST( ReferencePollutionErrorsTest_SelfReferencePollution )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &mut x; }
		fn Foo( S &mut s'x' ) ' x <- x '
		{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::SelfReferencePollution );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( ReferencePollutionErrorsTest_ArgReferencePollution )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &mut x; }
		fn Foo( S &mut s'x', i32 &'y mut r ) ' y <- x '
		{}
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArgReferencePollution );
	U_TEST_ASSERT( error.file_pos.line == 3u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( error.file_pos.line == 9u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( build_result.errors.size() >= 2u );

	U_TEST_ASSERT( build_result.errors[0].code == CodeBuilderErrorCode::DestroyedVariableStillHaveReferences );
	U_TEST_ASSERT( build_result.errors[1].code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( build_result.errors[1].file_pos.line == 10u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnallowedReferencePollution );
	U_TEST_ASSERT( error.file_pos.line == 12u );
}

U_TEST( ReferencePollutionErrorsTest_ReferencePollutionForArgReference_Test0 )
{
	static const char c_program_text[]=
	R"(
		struct S{ i32 &imut x; }
		struct P{ S &mut x; }
		fn FakePollution( S &mut s'x', i32 &'y i ) ' x <- y ' // reference pollution allowed in signature, but actually not happens.
		{}

		fn Foo( P &mut p'x', i32 &'y r )
		{
			FakePollution( p.x, r );
		} // Error, pollution of "p" with "r", which is not allowed.
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferencePollutionForArgReference );
	U_TEST_ASSERT( error.file_pos.line == 9u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitReferencePollutionForCopyConstructor );
	U_TEST_ASSERT( error.file_pos.line == 5u );
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

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExplicitReferencePollutionForCopyAssignmentOperator );
	U_TEST_ASSERT( error.file_pos.line == 5u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_0 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 x'a, b, c' ); // tag list for fundamental type
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidReferenceTagCount );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_1 )
{
	static const char c_program_text[]=
	R"(
		struct S{}
		fn Foo( S s'a, b, c' ); // tag list for struct without references
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidReferenceTagCount );
	U_TEST_ASSERT( error.file_pos.line == 3u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_2 )
{
	static const char c_program_text[]=
	R"(
		fn Foo( i32 &'x a, i32 &'y b, i32 &'z c ) : i32'x, y, z'; // tag list for return value
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidReferenceTagCount );
	U_TEST_ASSERT( error.file_pos.line == 2u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_3 )
{
	static const char c_program_text[]=
	R"(
		// Invalid tag list for complete type.
		struct S{}
		fn Foo( S s' a, b, c ' );
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidReferenceTagCount );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_4 )
{
	static const char c_program_text[]=
	R"(
		// Invalid tag list for incomplete type. Must NOT generate error, because function have no body.
		struct S;
		fn Foo( S s'a, b, c' );
	)";

	BuildProgram( c_program_text );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_5 )
{
	static const char c_program_text[]=
	R"(
		// Invalid tag list for complete type.
		struct S{}
		fn Foo() : S' a, b, c ';
	)";

	const ICodeBuilder::BuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InvalidReferenceTagCount );
	U_TEST_ASSERT( error.file_pos.line == 4u );
}

U_TEST( InnerTagsErrorsTest_InvalidReferenceTagCount_6 )
{
	static const char c_program_text[]=
	R"(
		// Invalid tag list for incomplete type. Must NOT generate error, because function have no body.
		struct S;
		fn Foo() : S'a, b, c';
	)";

	BuildProgram( c_program_text );
}

} // namespace U
