from py_tests_common import *


def BlockLabelDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test2():
	c_program_text= """
		fn Foo()
		{
			safe
			{
			} label some
		}
	"""
	tests_lib.build_program( c_program_text )


def BlockLabelDeclaration_Test3():
	c_program_text= """
		fn Foo()
		{
			if( true )
			{
				while( false )
				{
					{
					} label lll
				}
			}
			else
			{
				{
				} label some
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def BreakFromBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			{
				var bool cond= true;
				if( cond )
				{
					break label block_end;
				}
				halt; // Is unreachable
			} label block_end
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			{
				auto mut breaked= false;
				for( auto mut i= 0; i < 100; ++i ) label outer_loop
				{
					while(true) label inner_loop
					{
						{
							if( i == 13 )
							{
								breaked= true;
								break label outer_block;
							}
							else
							{
								break label inner_loop;
							}
						} label inner_block
					}
					halt if( i > 66 );
				}
				halt if( !breaked );
			} label outer_block
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test2():
	c_program_text= """
		fn Foo()
		{
			{
				return;
			} label block_end // This label is unreachable
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def BreakFromBlock_Test3():
	c_program_text= """
		fn Foo()
		{
			{
			} label block_end // This label is reachable only via normal control flow - without any "break".
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )
