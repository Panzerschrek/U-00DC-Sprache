from py_tests_common import *


def TypesMismatch_ForSwitchOperator_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				13u -> {}, // Expected i32, got u32
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 6 ) )


def TypesMismatch_ForSwitchOperator_Test1():
	c_program_text= """
		fn Foo( char8 c )
		{
			switch(c)
			{
				'7'c16 -> {}, // Expected char8, got char16
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 6 ) )


def TypesMismatch_ForSwitchOperator_Test2():
	c_program_text= """
		enum E{ A, B, C }
		fn Foo( E e )
		{
			switch(e)
			{
				1 -> {}, // Expected enum E, got i32
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 7 ) )


def TypesMismatch_ForSwitchOperator_Test3():
	c_program_text= """
		fn Foo( f32 x )
		{
			switch(x) // switch for "f32" isn't possible
			{
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ExpectedConstantExpression_ForSwitchOperator_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				Bar() -> {}, // Call to non-constexpr function
				default -> {},
			}
		}
		fn Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ExpectedConstantExpression", 6 ) )


def ExpectedConstantExpression_ForSwitchOperator_Test1():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				1 ... Bar() -> {}, // Call to non-constexpr function in range
				Bar() + 1 ... -> {}, // Call to non-constexpr function in range
				default -> {},
			}
		}
		fn Bar() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "ExpectedConstantExpression", 6 ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 7 ) )


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
	assert( HasError( errors_list, "SwitchDuplicatedDefaultLabel", 7 ) )


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
	assert( HasError( errors_list, "SwitchDuplicatedDefaultLabel", 9 ) )


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
	assert( HasError( errors_list, "SwitchDuplicatedDefaultLabel",  9 ) )
	assert( HasError( errors_list, "SwitchDuplicatedDefaultLabel", 11 ) )


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
	assert( HasError( errors_list, "SwitchInvalidRange", 6 ) )


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
	assert( HasError( errors_list, "SwitchInvalidRange", 6 ) )


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
	assert( HasError( errors_list, "SwitchInvalidRange", 6 ) )
	assert( HasError( errors_list, "SwitchInvalidRange", 7 ) )


def SwitchInvalidRange_Test4():
	c_program_text= """
		fn Foo( char8 c )
		{
			switch(c)
			{
				'g' ... 'f'  -> {}, // Invalid range for char
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "SwitchInvalidRange", 6 ) )


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
	assert( HasError( errors_list, "SwitchInvalidRange", 7 ) )


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
	assert( HasError( errors_list, "SwitchRangesOverlapping", 7 ) )


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
	assert( HasError( errors_list, "SwitchRangesOverlapping", 6 ) )


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
	assert( HasError( errors_list, "SwitchRangesOverlapping", 7 ) )


def SwitchRangesOverlapping_Test3():
	c_program_text= """
		fn Foo( char8 x )
		{
			switch(x)
			{
				'a' ... 'z' -> {},
				'p' ... 's' -> {}, // Subrange
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "SwitchRangesOverlapping", 7 ) )


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
	assert( HasError( errors_list, "SwitchRangesOverlapping", 8 ) )


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
	assert( HasError( errors_list, "SwitchRangesOverlapping", 7 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledValue", 8 ) )
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
	assert( HasError( errors_list, "SwitchUndhandledValue", 8 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledValue", 8 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledValue", 7 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledValue", 5 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledRange", 8 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledRange", 8 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledRange", 8 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledRange", 7 ) )


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
	assert( HasError( errors_list, "SwitchUndhandledRange", 5 ) )


def SwitchUnreachableDefaultBranch_Test0():
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
	assert( HasError( errors_list, "SwitchUnreachableDefaultBranch", 9 ) )


def SwitchUnreachableDefaultBranch_Test1():
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
	assert( HasError( errors_list, "SwitchUnreachableDefaultBranch", 6 ) )


def SwitchUnreachableDefaultBranch_Test2():
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
	assert( HasError( errors_list, "SwitchUnreachableDefaultBranch", 7 ) )


def NoBreakForSwitch_Test0():
	c_program_text= """
		fn Foo( i32 x )
		{
			switch(x)
			{
				0 ... -> { break; },
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "BreakOutsideLoop", 6 ) )
