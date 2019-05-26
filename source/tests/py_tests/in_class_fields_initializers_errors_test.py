from py_tests_common import *


def InClassFieldInitializerCheck_Test0():
	c_program_text= """
		struct S
		{
			i32 x= unknown_name;
			fn constructor() ( x= 0 ) {} // Prevent calling of initializer in default constructor generation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "NameNotFound" )
	assert( errors_list[0].file_pos.line == 4 )


def InClassFieldInitializerCheck_Test1():
	c_program_text= """
		auto constant= 3.14f;
		struct S
		{
			i32 x= constant;
			fn constructor() ( x= 0 ) {}  // Prevent calling of initializer in default constructor generation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )


def InClassFieldInitializerCheck_Test2():
	c_program_text= """
		struct S
		{
			i32& x= 42;
			fn constructor( this'a', i32&'b in_x ) ' a <- imut b '
			( x= in_x )
			{}  // Prevent calling of initializer in default constructor generation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 4 )


def InClassFieldInitializerCheck_Test3():
	c_program_text= """
	struct S
	{
		i32 x[ ];
		fn constructor() ( x= 0 ) {}  // Prevent calling of initializer in default constructor generation.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ArrayInitializerForNonArray" )
	assert( errors_list[0].file_pos.line == 4 )
