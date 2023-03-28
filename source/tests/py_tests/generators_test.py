from py_tests_common import *


def SimpleGenerator_Test0():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 42;
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 42 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
			}
			halt if( advanced );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test1():
	c_program_text= """
		fn generator SimpleGen() : i32
		{
			yield 24;
			yield 13;
			yield -678;
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 24 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != 13 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
				halt if( x != -678 );
			}
			halt if( !advanced );

			advanced= false;
			if_coro_advance( x : gen )
			{
				advanced= true;
			}
			halt if( advanced );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def SimpleGenerator_Test2():
	c_program_text= """
		fn generator SimpleGen() : u32
		{
			for( auto mut i= 0u; i < 12u; ++i )
			{
				yield i * i;
			}
		}
		fn Foo()
		{
			auto mut gen= SimpleGen();
			auto mut num_advanced= 0u;
			while( true )
			{
				if_coro_advance( x : gen )
				{
					halt if( x != num_advanced * num_advanced );
					++num_advanced;
					continue;
				}
				break;
			}
			halt if( num_advanced != 12u );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
