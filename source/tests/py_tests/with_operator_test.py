from py_tests_common import *

def WithOperatorDeclaration_Test0():
	c_program_text= """
		fn Foo()
		{
			with( x : 0 ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test1():
	c_program_text= """
		fn Foo()
		{
			with( & y : 0 )
			{
				return;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorDeclaration_Test2():
	c_program_text= """
		fn Foo( i32 mut a )
		{
			with( &mut z : a )
			{
				z*= 2;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def WithOperatorForValue_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			with( x : 6661 ) // Bind value
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 6661 )


def WithOperatorForValue_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			auto s= 9514789;
			with( x : s ) // Copy constant reference to value.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9514789 )


def WithOperatorForValue_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto mut s= 3214;
			with( x : s ) // Copy conent of mutable reference to value.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3214 )


def WithOperatorForValue_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 8741 };
			return s;
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			with( s : GetS() ) // Should move 's' here.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 8741 )


def WithOperatorForValue_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			var S s_src{ .x= 987789 };
			with( s : s_src ) // Should copy 's_src' here.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 987789 )


def WithOperatorForValue_Test5():
	c_program_text= """
		fn Foo() : i32
		{
			with( mut x : 9991 ) // Create mutable value.
			{
				x+= 8;
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 9999 )


def WithOperatorForImutReference_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			with( &imut x : 333222111 ) // Bind value to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 333222111 )


def WithOperatorForImutReference_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			auto s= 852741;
			with( &imut x : s ) // Bind immutable reference to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 852741 )


def WithOperatorForImutReference_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto mut s= 963147;
			with( &imut x : s ) // Bind mutable reference to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 963147 )


def WithOperatorForImutReference_Test3():
	c_program_text= """
		fn Foo()
		{
			auto mut s= 963147;
			with( &imut x : s )
			{
				++s; // Error, 's' have reference - 'x'.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 7 )


def WithOperatorForImutReference_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 654123 };
			return s;
		}
		fn Foo() : i32
		{
			with( &imut s : GetS() ) // Bind temporary value to immutable reference.
			{
				return s.x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654123 )


def WithOperatorForImutReference_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 111118 };
			return s;
		}
		fn Foo() : i32
		{
			with( &imut x : GetS().x ) // Bind part of temporary value to immutable reference.
			{
				return x;
			}
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 111118 )


def WithOperatorForImutReference_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			fn destructor(){ x= 0; }
		}
		fn GetS() : S
		{
			var S s{ .x= 999996 };
			return s;
		}
		fn Foo() : i32
		{
			var i32 mut res= zero_init;
			with( &imut s : GetS() ) // Bind temporary value to immutable reference.
			{
				res= s.x; // Should read member here before 's' destruction.
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 999996 )


def WithOperatorForMutReference_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut res= 0;
			with( &mut x : res )
			{
				x= 29567;
			}
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 29567 )
