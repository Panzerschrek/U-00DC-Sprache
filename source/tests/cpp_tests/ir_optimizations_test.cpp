#include "cpp_tests.hpp"

namespace U
{

namespace
{

U_TEST(AutoMoveInitializationOptimization_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S{ [i32, 4] x; }
		fn Foo()
		{
			var S mut s0= zero_init;
			auto s0_address= $<(s0);
			// Storage of stack variable "s0" must be reused here for "s1".
			auto mut s1= move(s0);
			auto s1_address= $<(s1);
			halt if(s0_address != s1_address);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(AutoMoveInitializationOptimization_Test1)
{
	static const char c_program_text[]=
	R"(
		struct S{ [i32, 4] x; }
		fn Bar( S mut s0 )
		{
			auto s0_address= $<(s0);
			// Storage of variable argument "s0" must be reused here for "s1".
			auto mut s1= move(s0);
			auto s1_address= $<(s1);
			halt if(s0_address != s1_address);
		}
		fn Foo()
		{
			var S mut s= zero_init;
			Bar( move(s) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(AutoMoveInitializationOptimization_Test2)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn Foo()
		{
			// Reuse storage of temporary variable in auto variable initialization.
			auto mut s= S();
			auto s_address= $<(s);
			halt if(s_address != s.self_ptr);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(WithOperatorVariableMoveInitializationOptimization_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S{ [i32, 4] x; }
		fn Foo()
		{
			var S mut s0= zero_init;
			auto s0_address= $<(s0);
			with ( mut s1 : move(s0) )
			{
				// Storage of stack variable "s0" must be reused here for "s1".
				auto s1_address= $<(s1);
				halt if(s0_address != s1_address);
			}
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(WithOperatorVariableMoveInitializationOptimization_Test1)
{
	static const char c_program_text[]=
	R"(
		struct S{ [i32, 4] x; }
		fn Bar( S mut s0 )
		{
			auto s0_address= $<(s0);
			with( mut s1 : move(s0) )
			{
				// Storage of variable argument "s0" must be reused here for "s1".
				auto s1_address= $<(s1);
				halt if(s0_address != s1_address);
			}
		}
		fn Foo()
		{
			var S mut s= zero_init;
			Bar( move(s) );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(MoveReturnVariableAllocationOptimization_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S{ [i32, 4] x; }
		fn GetS( $(S) &mut s_ptr ) : S
		{
			// "s" must be allocated in place of "s_ret" (hidden) pointer argument.
			var S mut s= zero_init;
			s_ptr= $<(s);
			return move(s);
		}
		fn Foo()
		{
			var $(S) mut ptr= zero_init;
			auto mut s= GetS( ptr );
			halt if(ptr != $<(s));
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(MoveReturnVariableAllocationOptimization_Test1)
{
	static const char c_program_text[]=
	R"(
	struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn GetS0() : S
		{
			return S();
		}
		fn GetS1() : S
		{
			// "s_ret" passed here as hidden argument of function. So, no copying is needed here.
			return GetS0();
		}
		fn Foo()
		{
			// Initial allocation of "s" must be the same because of return variable address optimization and auto move initialization optimization.
			auto mut s= GetS1();
			halt if($<(s) != s.self_ptr);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(MoveReturnVariableAllocationOptimization_Test2)
{
	static const char c_program_text[]=
	R"(
	struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn GetS(bool b) : S
		{
			// Allocation of result variable in "s_ret" here must be used in all return paths, because single variable used inside them.
			var S mut s;
			if(b)
			{
				return move(s);
			}
			s.payload= 1.0f;

			return move(s);
		}

		fn Bar(bool b)
		{
			auto mut s= GetS(b);
			halt if($<(s) != s.self_ptr);
		}

		fn Foo()
		{
			Bar(true);
			Bar(false);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(MoveReturnVariableAllocationOptimization_Test3)
{
	static const char c_program_text[]=
	R"(
	struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn GetS(bool b) : S
		{
			// Can't perform here return value optimization - returning different local variables.
			var S mut s0, mut s1;
			halt if($<(s0) == $<(s1));
			if(b)
			{
				return move(s0);
			}
			else
			{
				return move(s1);
			}
		}

		fn Foo()
		{
			auto mut s0= GetS(false);
			auto mut s1= GetS(true);
			halt if( $<(s0) == s0.self_ptr );
			halt if( $<(s1) == s1.self_ptr );
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(MoveReturnVariableAllocationOptimization_Test4)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn Pass(S mut s) : S
		{
			// Memcpy should be executed here, because there is no way to reallocate value argument in "s_ret"
			return move(s);
		}
		fn Foo()
		{
			auto mut s= Pass(S());
			halt if($<(s) == s.self_ptr);
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(ArgumentVariableAllocationOptimization_Test0)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn Bar(S mut s)
		{
			halt if($<(s) != s.self_ptr);
		}
		fn Foo()
		{
			Bar(S()); // Address of temp variable of type "S" passed directly into function here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(ArgumentVariableAllocationOptimization_Test1)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn Bar(S & s)
		{
			unsafe {  halt if( $<( cast_mut(s) ) != s.self_ptr );  }
		}
		fn Foo()
		{
			Bar(S()); // Address of temp variable of type "S" passed directly into function here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

U_TEST(ArgumentVariableAllocationOptimization_Test2)
{
	static const char c_program_text[]=
	R"(
		struct S
		{
			[ u64, 8 ] dummy;
			$(S) self_ptr;
			f32 payload;

			fn constructor() ( dummy= zero_init, self_ptr= zero_init, payload= zero_init )
			{
				self_ptr= $<(this);
			}
		}
		fn GetS() : S
		{
			return S();
		}
		fn Bar(S mut s)
		{
			halt if($<(s) != s.self_ptr);
		}
		fn Foo()
		{
			Bar(GetS()); // Address of call result variable of type "S" passed directly into function here.
		}
	)";

	const EnginePtr engine= CreateEngine( BuildProgram( c_program_text ) );

	llvm::Function* const function= engine->FindFunctionNamed( "_Z3Foov" );
	U_TEST_ASSERT( function != nullptr );

	engine->runFunction( function, {} );
}

} // namespace

} // namespace U
