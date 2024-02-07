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
	assert( HasError( errors_list, "NameNotFound", 4 ) )


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
	assert( errors_list[0].src_loc.line == 5 )


def InClassFieldInitializerCheck_Test2():
	c_program_text= """
		struct S
		{
			i32& x= 42;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32& in_x ) @(pollution)
			( x= in_x )
			{}  // Prevent calling of initializer in default constructor generation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 4 )


def InClassFieldInitializerCheck_Test4():
	c_program_text= """
	class C
	{
	private:
		auto constant= 666;
	}
	struct S
	{
		i32 x= C::constant;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "AccessingNonpublicClassMember" )
	assert( errors_list[0].src_loc.line == 9 )


def InClassFieldInitializer_UnsafeInitializerDisabled_Test0():
	c_program_text= """
	struct S
	{
		i32 x= uninitialized;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "UninitializedInitializerOutsideUnsafeBlock" )
	assert( errors_list[0].src_loc.line == 4 )


def InClassFieldInitializer_OtherFieldCanNotBeUsed_Test0():
	c_program_text= """
	struct S
	{
		i32 x= 0;
		i32 y= x;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ClassFieldAccessInStaticMethod", 5 ) )


def InClassFieldInitializer_OtherFieldCanNotBeUsed_Test1():
	c_program_text= """
	struct S
	{
		i32 x= y;
		i32 y= 0;
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ClassFieldAccessInStaticMethod", 4 ) )
