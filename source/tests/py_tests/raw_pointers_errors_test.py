from py_tests_common import *

def ValueIsNotReference_Test0():
	c_program_text= """
		fn Foo()
		{
			$<(42); // Converting immediate value to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotReference", 4 ) )


def ValueIsNotReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 x= 66, y= -5;
			$<(x + y); // Converting binary operator result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotReference", 5 ) )


def ValueIsNotReference_Test2():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			$<(S()); // Converting  temp variable construction result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotReference", 5 ) )


def ValueIsNotReference_Test3():
	c_program_text= """
		fn Bar() : tup[ f32, bool ]
		{
			var tup[ f32, bool ] t[ 0.25f, true ];
			return t;
		}
		fn Foo()
		{
			$<(Bar()); // Converting function call result (immediate value) to pointer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotReference", 9 ) )


def RawPointerToReferenceConversionOutsideUnsafeBlock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			auto ptr= $<(x);
			$>(ptr);
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "RawPointerToReferenceConversionOutsideUnsafeBlock", 6 ) )


def RawPointerToReferenceConversionOutsideUnsafeBlock_Test1():
	c_program_text= """
		fn Foo()
		{
			unsafe
			{
				auto mut x= 0;
				auto ptr= $<(x);
				safe
				{
					$>(ptr);
				}
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "RawPointerToReferenceConversionOutsideUnsafeBlock", 10 ) )


def ValueIsNotPointer_Test0():
	c_program_text= """
		fn Foo()
		{
			$>(0); // Expected pointer, got value of fundamental type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotPointer", 4 ) )


def ValueIsNotPointer_Test1():
	c_program_text= """
		fn Foo()
		{
			var size_type s= 0;
			$>(s); // Expected pointer, got reference of fundamental type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotPointer", 5 ) )


def ValueIsNotPointer_Test2():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var S s= zero_init;
			$>(s); // Expected pointer, got reference of struct type.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotPointer", 6 ) )


def ValueIsNotPointer_Test3():
	c_program_text= """
		fn Foo()
		{
			var (fn()) this_ptr(Foo);
			$>(this_ptr); // Expected pointer, got function pointer (which is different kind of type).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ValueIsNotPointer", 5 ) )
