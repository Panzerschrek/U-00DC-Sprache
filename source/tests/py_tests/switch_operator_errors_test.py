from py_tests_common import *


def SwitchDuplicatedDefaultLabel_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				default -> {},
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchDuplicatedDefaultLabel", 7 ) )


def SwitchDuplicatedDefaultLabel_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				0 -> {},
				default -> {},
				1 -> {},
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchDuplicatedDefaultLabel", 9 ) )


def SwitchDuplicatedDefaultLabel_Test2():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				0 -> {},
				default -> {},
				1 -> {},
				default -> {},
				2 -> {},
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchDuplicatedDefaultLabel",  9 ) )
	assert( HaveError( errors_list, "SwitchDuplicatedDefaultLabel", 11 ) )


def SwitchInvalidRange_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				-7 ... -80 -> {}, // Invalid signed negative range
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchInvalidRange", 6 ) )


def SwitchInvalidRange_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				77 ... 76 -> {}, // Invalid signed positive range
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchInvalidRange", 6 ) )


def SwitchInvalidRange_Test2():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				888 ... 888 -> {}, // Ok - correct range
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def SwitchInvalidRange_Test3():
	c_program_text= """
		fn Foo( u32 x )
		{
			switch(x)
			{
				80u ... 79u -> {}, // Invalid unsigned range
				3000000000u ... 2500000000u -> {}, // Invalid unsigned large range
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HaveError( errors_list, "SwitchInvalidRange", 6 ) )
	assert( HaveError( errors_list, "SwitchInvalidRange", 7 ) )


def SwitchInvalidRange_Test4():
	c_program_text= """
		fn Foo( char8 c )
		{
			switch(c)
			{
				"g"c8 ... "f"c8  -> {}, // Invalid range for cha
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchInvalidRange", 6 ) )


def SwitchInvalidRange_Test5():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				E::C ... E::A -> {}, // Invalid range for enum.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchInvalidRange", 7 ) )
