from py_tests_common import *


def IfAlternatives_Test0():
	# Append "static_if" to "if".
	c_program_text= """
		template</bool static_cond/> fn Some( bool dynamic_cond ) : i32
		{
			if( dynamic_cond )
			{
				return 66;
			}
			else static_if( static_cond )
			{
				return 77;
			}
			else
			{
				return 88;
			}
		}

		fn Foo()
		{
			halt if( Some</false/>(false) != 88 );
			halt if( Some</false/>(true) != 66 );
			halt if( Some</true/>(false) != 77 );
			halt if( Some</true/>(true) != 66 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def IfAlternatives_Test1():
	# Append "if" to "static_if".
	c_program_text= """
		template</bool static_cond/> fn Some( bool dynamic_cond ) : i32
		{
			static_if( static_cond )
			{
				return 999;
			}
			else if( dynamic_cond )
			{
				return 888;
			}
			else
			{
				return 777;
			}
		}

		fn Foo()
		{
			halt if( Some</false/>(false) != 777 );
			halt if( Some</false/>(true) != 888 );
			halt if( Some</true/>(true) != 999 );
			halt if( Some</true/>(true) != 999 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def IfAlternatives_Test2():
	# Append "static_if" to another "static_if".
	c_program_text= """
		template</bool c0, bool c1/> fn Some() : i32
		{
			static_if( c0 )
			{
				return 123;
			}
			else if( c1 )
			{
				return 456;
			}
			else
			{
				return 789;
			}
		}

		fn Foo()
		{
			halt if( Some</false, false/>() != 789 );
			halt if( Some</false,  true/>() != 456 );
			halt if( Some</ true, false/>() != 123 );
			halt if( Some</ true,  true/>() != 123 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def IfAlternatives_Test3():
	# Append "else" to "if_coro_advance"
	c_program_text= """
		fn generator SomeGen() : i32
		{
			yield 0; yield 1; yield 2;
		}

		fn Foo()
		{
			auto mut gen= SomeGen();
			auto mut advanced= 0;
			while(true)
			{
				if_coro_advance( x : gen )
				{
					halt if( x != advanced );
					++advanced;
				}
				else
				{
					break;
				}
			}
			halt if( advanced != 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )



def IfAlternatives_Test4():
	# Append "else if" to "if_coro_advance"
	c_program_text= """
		fn generator SomeGen() : i32
		{
			yield 10; yield 11; yield 12;
		}

		fn Foo()
		{
			auto mut gen= SomeGen();
			auto mut advanced= 0;
			while(true)
			{
				if_coro_advance( x : gen )
				{
					halt if( x != 10 + advanced );
					++advanced;
				}
				else if( advanced < 10 )
				{
					advanced += 55;
					break;
				}
			}
			halt if( advanced != 55 + 3 );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def IfAlternatives_Test5():
	# Append "if_coro_advance if" to "static_if"
	c_program_text= """
		fn Foo()
		{
			static_if( true )
			{
			}
			else if_coro_advance( x : unknown_coro )
			{
				call_unknown_function();
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
