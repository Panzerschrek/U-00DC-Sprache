#include "tests.hpp"

namespace U
{

U_TEST( BasicFunctionManglingTest )
{
	static const char c_program_text[]=
	R"(
		fn Foo(){} // void return, void args
		fn Bar() : i32 { return 0; } // non-void return, void args
		fn Baz(i32 x) {} // void return, non-empty args
		fn Lol(i32 x) : f32 { return f32(x); } // non-void return, non-empty args
		fn SeveralArgs(u64 x, f32 y, bool z){} // void return, several args
		fn ArgsAndRet(u32 x, i16 y, char8 z) : f64 { return 0.0; }

		fn RefArg(i32& x) {} // imut reference arg
		fn MutRefArg(f32 &mut x){} // mut reference arg

		fn RefRet( f64& x ) : f64& { return x; } // imut return reference
		fn MutRefRet( i32 &mut x ) : i32 &mut { return x; } // mut return reference

		fn MutValueArg(i32 mut x ){}
		fn ImutValueArg(i32 imut x){}
	)";

	const EnginePtr engine= CreateEngine( BuildProgramForMSVCManglingTest( c_program_text ) );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?Foo@@YAXXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Bar@@YAHXZ" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Baz@@YAXH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?Lol@@YAMH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?SeveralArgs@@YAX_KM_N@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ArgsAndRet@@YANIFD@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefArg@@YAXAEBH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefArg@@YAXAEAM@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?RefRet@@YAAEBNAEBN@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutRefRet@@YAAEAHAEAH@Z" ) != nullptr );

	U_TEST_ASSERT( engine->FindFunctionNamed( "?MutValueArg@@YAXH@Z" ) != nullptr );
	U_TEST_ASSERT( engine->FindFunctionNamed( "?ImutValueArg@@YAXH@Z" ) != nullptr );
}

} // namespace U
