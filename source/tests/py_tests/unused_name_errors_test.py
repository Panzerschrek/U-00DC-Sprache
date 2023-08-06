from py_tests_common import *


def UnusedLocalTypeAlias_Test0():
	c_program_text= """
		fn Foo()
		{
			type I= i32;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test1():
	c_program_text= """
		fn Foo()
		{
			type SS= SomeStruct;
		}
		struct SomeStruct{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test2():
	c_program_text= """
		fn Foo()
		{
			type SC= SomeClass;
		}
		class SomeClass polymorph{}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 4 ) )


def UnusedLocalTypeAlias_Test3():
	c_program_text= """
		fn Foo()
		{
			{
				type AliasInsideScope= void;
			}
		}	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 5 ) )


def UnusedLocalTypeAlias_Test4():
	c_program_text= """
		fn Foo(bool cond)
		{
			if(cond)
			{
				type AliasInsideIf= bool;
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )


def UnusedLocalTypeAlias_Test5():
	c_program_text= """
		fn Foo()
		{
			while(Bar())
			{
				type AliasInsideLoop= typeof(666);
			}
		}
		fn Bar() : bool;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text, True ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "UnusedName", 6 ) )
