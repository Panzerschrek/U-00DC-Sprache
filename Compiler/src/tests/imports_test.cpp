#include "tests.hpp"

namespace U
{

U_TEST( ImportsTest0 )
{
	static const char c_program_text_a[]=
	R"(
		fn Bar() : i32
		{
			return  586;
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		fn Foo() : i32
		{
			return Bar();
		}
	)";

	BuildMultisourceProgram(
		{
			{ "a"_SpC, c_program_text_a },
			{ "root"_SpC, c_program_text_root }
		},
		"root"_SpC );
}

U_TEST( ImportsTest1_FunctionPrototypeInOneFileAndBodyInAnother )
{
	static const char c_program_text_a[]=
	R"(
		fn Bar() : i32;
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		fn Bar() : i32
		{
			return  586;
		}

		fn Foo() : i32
		{
			return Bar();
		}
	)";

	const EnginePtr engine=
		CreateEngine(
			BuildMultisourceProgram(
				{
					{ "a"_SpC, c_program_text_a },
					{ "root"_SpC, c_program_text_root }
				},
				"root"_SpC ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(586) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ImportsTest2_FunctionsWithDifferentSignaturesInDifferentFiles )
{
	static const char c_program_text_a[]=
	R"(
		fn Bar( f32 x ) : i32
		{
			return 8854;
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"

		fn Bar( i32 x ) : i32
		{
			return 984364;
		}

		fn Foo() : i32
		{
			return Bar( 0.0f );
		}
	)";

	const EnginePtr engine=
		CreateEngine(
			BuildMultisourceProgram(
				{
					{ "a"_SpC, c_program_text_a },
					{ "root"_SpC, c_program_text_root }
				},
				"root"_SpC ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(8854) == result_value.IntVal.getLimitedValue() );
}

U_TEST( ImportsTest3_MultipleInportOfSameFile_Test0 )
{
	static const char c_program_text_a[]=
	R"(
		fn Bar() : i32
		{
			return  586;
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "a" // Should discard this import.

		fn Foo() : i32
		{
			return Bar();
		}
	)";

	BuildMultisourceProgram(
		{
			{ "a"_SpC, c_program_text_a },
			{ "root"_SpC, c_program_text_root }
		},
		"root"_SpC );
}

U_TEST( ImportsTest4_MultipleInportOfSameFile_Test1 )
{
	static const char c_program_text_a[]=
	R"(
		struct X{}
	)";

	static const char c_program_text_b[]=
	R"(
		import "a"
		struct Y{}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		import "b" // "b" already contains "a", so, it does not loads twice.
	)";

	BuildMultisourceProgram(
		{
			{ "a"_SpC, c_program_text_a },
			{ "b"_SpC, c_program_text_b },
			{ "root"_SpC, c_program_text_root }
		},
		"root"_SpC );
}

U_TEST( ImportsTest5_ImportContentOfNamespace )
{
	static const char c_program_text_a[]=
	R"(
		namespace X
		{
			fn Bar() : i32 { return 44512; }
		}
	)";

	static const char c_program_text_root[]=
	R"(
		import "a"
		fn Foo() : i32
		{
			return X::Bar();
		}
	)";

	const EnginePtr engine=
		CreateEngine(
			BuildMultisourceProgram(
				{
					{ "a"_SpC, c_program_text_a },
					{ "root"_SpC, c_program_text_root }
				},
				"root"_SpC ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	const llvm::GenericValue result_value= engine->runFunction( function, llvm::ArrayRef<llvm::GenericValue>() );

	U_TEST_ASSERT( static_cast<uint64_t>(44512) == result_value.IntVal.getLimitedValue() );
}

} // namespace U
