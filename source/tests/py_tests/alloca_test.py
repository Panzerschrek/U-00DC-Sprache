from py_tests_common import *


def AllocaOperator_Test0():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				// Size is static.
				var $(f64) mem= alloca</f64/>(32s);
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def AllocaOperator_Test1():
	c_program_text= """
		fn Foo(u32 size)
		{
			unsafe
			{
				// Size is dynamic.
				var $(u16) mem= alloca</u16/>( size_type(size) );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 128 )


def AllocaOperator_Test2():
	c_program_text= """
		fn Foo(u32 size)
		{
			// Conditional "alloca"
			if( size % 2u == 0u )
			{
				unsafe
				{
					var $(char32) mem= alloca</ char32 />( size_type(size) );
				}
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Fooj", 33 )
	tests_lib.run_function( "_Z3Fooj", 34 )


def AllocaOperator_Test3():
	c_program_text= """
		// Class with no constructor and destructor which halts.
		class C
		{
			i32 field;

			fn destructor()
			{
				halt;
			}
		}
		fn Foo()
		{
			unsafe
			{
				// Memory is allocated, but no constructors/destructors are called.
				var $(C) mem= alloca</ C />( 20s );
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
