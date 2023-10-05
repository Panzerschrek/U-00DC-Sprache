from py_tests_common import *

def ByValThis_Declaeation_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaeation_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval imut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Declaeation_Test2():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def ByValThis_Test0():
	c_program_text= """
		struct S
		{
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		fn Foo() : i32
		{
			var S s{ .x= 765 };
			return s.Foo();
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 765 )


def ByValThis_Test1():
	c_program_text= """
		struct S
		{
			fn Foo( byval mut this ) { x= 0; }
			i32 x;
		}
		fn Foo() : i32
		{
			var S mut s{ .x= 123 };
			s.Foo(); // Modification doesn't affect source, since "this" is passed by value.
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 123 )


def ByValThis_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn Foo() : i32
		{
			var S mut s{ .x= 88776655 };
			return move(s).Foo(); // Call by-value method with immediate value.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88776655 )


def ByValThis_Test2():
	c_program_text= """
		struct S
		{
			fn constructor( mut this, S& other )= delete;
			fn Foo( byval this ) : i32 { return x; }
			i32 x;
		}
		static_assert( !typeinfo</S/>.is_copy_constructible );
		fn Foo() : i32
		{
			var S s{ .x= 88776655 };
			return s.Foo(); // Error - trying to copy-construct value in byval-this method call.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "CopyConstructValueOfNoncopyableType", 12 ) )
