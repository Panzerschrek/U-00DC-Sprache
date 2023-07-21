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
				default -> {}
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


def SwitchRangesOverlapping_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				123 -> {},
				123 -> {}, // Duplicated value
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 7 ) )


def SwitchRangesOverlapping_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				123, 100 + 23  -> {}, // Duplcated value in same branch.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 6 ) )


def SwitchRangesOverlapping_Test2():
	c_program_text= """
		fn Foo( u32 x )
		{
			switch(x)
			{
				77u -> {},
				0u ... 100u -> {}, // Single value inside range.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 7 ) )


def SwitchRangesOverlapping_Test3():
	c_program_text= """
		fn Foo( char8 x )
		{
			switch(x)
			{
				"a"c8 ... "z"c8 -> {},
				"p"c8 ... "s"c8 -> {}, // Subrange
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 7 ) )


def SwitchRangesOverlapping_Test4():
	c_program_text= """
		enum E{ A, B, C, D, E, F }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A ... E::C  -> {},
				E::B ... E::F  -> {}, // intersection of ranges
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 8 ) )


def SwitchRangesOverlapping_Test5():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				0 ... 10 -> {},
				10 ... 20 -> {}, // Touching ranges.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchRangesOverlapping", 7 ) )


def SwitchUndhandledValue_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				... 41 -> {},
				// Single unhandled value "42" here.
				43 ... -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledValue", 8 ) )
	assert( errors_list[0].text.find("42") != -1 )


def SwitchUndhandledValue_Test1():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A -> {},
				E::C -> {},
				// B - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledValue", 8 ) )


def SwitchUndhandledValue_Test2():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A -> {},
				E::B -> {},
				// C - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledValue", 8 ) )


def SwitchUndhandledValue_Test3():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				E::B -> {},
				E::C -> {},
				// A - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledValue", 7 ) )


def SwitchUndhandledValue_Test4():
	c_program_text= """
		enum E{ A }
		fn Foo( E e )
		{
			switch(e)
			{
				// A - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledValue", 5 ) )


def SwitchUndhandledRange_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				... 41 -> {},
				// Unhandled range [42; 43] here.
				44 ... -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledRange", 8 ) )


def SwitchUndhandledRange_Test1():
	c_program_text= """
		enum E{ A, B, C, D }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A -> {},
				E::D -> {},
				// [B; C] - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledRange", 8 ) )


def SwitchUndhandledRange_Test2():
	c_program_text= """
		enum E{ A, B, C, D }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A -> {},
				E::B -> {},
				// [C; D] - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledRange", 8 ) )


def SwitchUndhandledRange_Test3():
	c_program_text= """
		enum E{ Null, A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				E::B -> {},
				E::C -> {},
				// [Null; A] - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledRange", 7 ) )


def SwitchUndhandledRange_Test4():
	c_program_text= """
		enum E{ A, B }
		fn Foo( E e )
		{
			switch(e)
			{
				// [A; B] - unhandled
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwitchUndhandledRange", 5 ) )


def SwithcUnreachableDefaultBranch_Test0():
	c_program_text= """
		enum E{ A, B }
		fn Foo( E e )
		{
			switch(e)
			{
				E::A -> {},
				E::B -> {},
				default -> {}, // Unreachable - all enum values are handled.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwithcUnreachableDefaultBranch", 9 ) )


def SwithcUnreachableDefaultBranch_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				default -> {}, // Unreachable - all numeric values are handled.
				... -1 -> {},
				0 -> {},
				1 ... -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwithcUnreachableDefaultBranch", 6 ) )


def SwithcUnreachableDefaultBranch_Test2():
	c_program_text= """
		fn Foo( char16 c )
		{
			switch(c)
			{
				... -> {}, // Hanlde all range here.
				default -> {}, // Unreachable - all numeric values are handled.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "SwithcUnreachableDefaultBranch", 7 ) )
