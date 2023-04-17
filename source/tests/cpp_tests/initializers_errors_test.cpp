#include <cstdlib>
#include <iostream>

#include "tests.hpp"

namespace U
{

namespace
{

U_TEST(ExpectedInitializerTest0)
{
	// Expected initializer for fundamental variable.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedInitializerTest1)
{
	// Expected initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 1024 ] x;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ExpectedInitializerTest2)
{
	// Expected initializer for struct.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; }
		fn Foo()
		{
			var S s;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ExpectedInitializerTest3)
{
	// Expected initializer for one of struct fields.
	static const char c_program_text[]=
	R"(
		struct S{ i32 x; i32 y; }
		fn Foo()
		{
			var S s{ .x= 678 }; // 'y' left uninitialized here
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ArrayInitializerForNonArrayTest0)
{
	// Array initializer for fundamental type.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x[ 5, 6, 7 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ArrayInitializerForNonArrayTest1)
{
	// Array initializer for structes.
	static const char c_program_text[]=
	R"(
		struct C{}
		fn Foo()
		{
			var C x[ 5, 6, 7 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializerForNonArray );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ArrayInitializersCountMismatchTest0)
{
	// Not enough initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 3u32 ] x[ 1 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ArrayInitializersCountMismatchTest1)
{
	// Too much initializers.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 3u32 ] x[ 1, 2, 3, 4, 5 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ArrayInitializersCountMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(FundamentalTypesHaveConstructorsWithExactlyOneParameterTest0)
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(FundamentalTypesHaveConstructorsWithExactlyOneParameterTest1)
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 x( 0, 1, 2 );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::FundamentalTypesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ReferencesHaveConstructorsWithExactlyOneParameterTest0)
{
	// Not enough parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 & x();
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(ReferencesHaveConstructorsWithExactlyOneParameterTest1)
{
	// Too much parameters in constructor.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 z= 0;
			var i32 & x( z, z );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ReferencesHaveConstructorsWithExactlyOneParameter );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(UnsupportedInitializerForReferenceTest0)
{
	// Array initializer for reference.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var i32 z= 0;
			var i32 & x[ z ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnsupportedInitializerForReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ConstructorInitializerForUnsupportedTypeTest0)
{
	// Constructor initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 2u32 ] x( 0, 1, 2 );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(StructInitializerForNonStructTest0)
{
	// Struct initializer for array.
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var [ i32, 2u32 ] x{};
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(StructInitializerForNonStructTest1)
{
	// Struct initializer for class. For classes initialization only constructors must be used.
	static const char c_program_text[]=
	R"(
		class FF{}
		fn Foo()
		{
			var FF ff{};
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::StructInitializerForNonStruct );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(InitializerForNonfieldStructMemberTest0)
{
	// Struct initializer for array.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn Foo( this ){}
		}
		fn Foo()
		{
			var S s{ .x= 0, .Foo= 0 };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerForNonfieldStructMember );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST(DuplicatedStructMemberInitializerTest0)
{
	static const char c_program_text[]=
	R"(
		struct Point{ i32 x; i32 y; }
		fn Foo()
		{
			var Point point{ .x= 42, .y= 34, .x= 0 };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DuplicatedStructMemberInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(DuplicatedStructMemberInitializerTest1)
{
	static const char c_program_text[]=
	R"(
		class A polymorph{}
		class B : A
		{
			fn constructor() ( base(), base() ) {}   // duplicated intitializer for base class
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::DuplicatedStructMemberInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(InitializerDisabledBecauseClassHaveExplicitNoncopyConstructorsTest0)
{
	// Struct named initializer for struct with constructor.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 a ) ( x=a ) {}
		}
		fn Foo()
		{
			var S point{ .x=42 };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST(InitializerDisabledBecauseClassHaveExplicitNoncopyConstructorsTest1)
{
	// Zero-initializer for struct with copy-constructor.
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			fn constructor( i32 imut a ) ( x(a) ) {}
		}
		fn Foo()
		{
			var S point= zero_init;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST(InitializerDisabledBecauseClassHaveExplicitNoncopyConstructorsTest2)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			i32 x;
			template</type T/> fn constructor( T a ) ( x(a) ) {}
		}
		fn Foo()
		{
			// Template constructor existis in this class, so, struct named initializer must be disabled.
			var S point{ .x= 0 };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors );
	U_TEST_ASSERT( error.src_loc.GetLine() == 10u );
}

U_TEST( InitializerForInvalidType_Test0 )
{
	// Type is invalid, because name of type not found.
	// expression initializer.
	static const char c_program_text[]=
	R"(
		var unknown_type constexpr x= 0;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( InitializerForInvalidType_Test1 )
{
	// Type is invalid, because name of type not found.
	// constructor initializer.
	static const char c_program_text[]=
	R"(
		var unknown_type constexpr x(0);
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( InitializerForInvalidType_Test2 )
{
	// Type is invalid, because name of type not found.
	// struct initializer.
	static const char c_program_text[]=
	R"(
		var unknown_type constexpr x{};
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( InitializerForInvalidType_Test3 )
{
	// Type is invalid, because name of type not found.
	// zero initializer.
	static const char c_program_text[]=
	R"(
		var unknown_type constexpr x= zero_init;
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST( InitializerForInvalidType_Test4 )
{
	// Type is invalid, because name of type not found.
	// array initializer.
	static const char c_program_text[]=
	R"(
		var [ unknown_type, 2 ] constexpr x[];
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::NameNotFound );
	U_TEST_ASSERT( error.src_loc.GetLine() == 2u );
}

U_TEST(ZeroInitializerForClass_Test0)
{
	// Struct initializer for class. For classes initialization only constructors must be used.
	static const char c_program_text[]=
	R"(
		class FF{}
		fn Foo()
		{
			var FF ff= zero_init;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ZeroInitializerForClass );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ZeroInitializerForReferenceField_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		fn Foo()
		{
			var S s= zero_init;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnsupportedInitializerForReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(ZeroInitializerForReferenceField_Test1)
{
	static const char c_program_text[]=
	R"(
		struct S{ i32& x; }
		fn Foo()
		{
			var S s{ .x= zero_init };
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::UnsupportedInitializerForReference );
	U_TEST_ASSERT( error.src_loc.GetLine() == 5u );
}

U_TEST(TuplesInitializersErrors_Test0)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ i32, bool ] t;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test1)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, f64, i64 ] t;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ExpectedInitializer );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test2)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn constructor(){}
		}
		fn Foo()
		{
			var tup[ f32, S, i64 ] t= zero_init;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::InitializerDisabledBecauseClassHaveExplicitNoncopyConstructors );
	U_TEST_ASSERT( error.src_loc.GetLine() == 8u );
}

U_TEST(TuplesInitializersErrors_Test3)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, bool, i64 ] t[ 0.5f ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TupleInitializersCountMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test4)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, bool, i64 ] t[ 0.5f, true ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TupleInitializersCountMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test5)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, bool ] t[ 0.5f, true, 666 ];
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::TupleInitializersCountMismatch );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test6)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn constructor( mut this, S&imut other )= delete;
		}
		fn Foo()
		{
			var tup[ f32, S ] t= zero_init;
			var tup[ f32, S ] t_copy(t); // Can not copy tuple, because tuple element "struct S" is not copyable.
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CopyConstructValueOfNoncopyableType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

U_TEST(TuplesInitializersErrors_Test7)
{
	static const char c_program_text[]=
	R"(
		fn Foo()
		{
			var tup[ f32, bool ] t( 0.5f, true );
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::ConstructorInitializerForUnsupportedType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 4u );
}

U_TEST(TuplesInitializersErrors_Test8)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			fn constructor( mut this, S &imut other )= delete;
		}
		fn Foo()
		{
			var tup[ S ] t0= zero_init;
			var tup[ S ] t1= t0;
		}
	)";

	const ErrorTestBuildResult build_result= BuildProgramWithErrors( c_program_text );

	U_TEST_ASSERT( !build_result.errors.empty() );
	const CodeBuilderError& error= build_result.errors.front();

	U_TEST_ASSERT( error.code == CodeBuilderErrorCode::CopyConstructValueOfNoncopyableType );
	U_TEST_ASSERT( error.src_loc.GetLine() == 9u );
}

} // namespace

} // namespace U
