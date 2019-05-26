from py_tests_common import *

def FieldInitializerDeclaration_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 0; // Expression initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test1():
	c_program_text= """
		struct S
		{
			i32 x(0); // Constructor initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			i32 x= zero_init; // Zero initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test2():
	c_program_text= """
		struct S
		{
			[ i32, 2 ] arr[ 55, 66 ]; // Array initializer
		}
	"""
	tests_lib.build_program( c_program_text )


def FieldInitializerDeclaration_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
		}
		struct T
		{
			S s{ .x= 0 }; // Struct named initializerg
		}
	"""
	tests_lib.build_program( c_program_text )


def InClassFieldInitializer_InStructNamedInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 66;
		}
		fn Foo() : i32
		{
			var S s{};
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 66 )


def InClassFieldInitializer_InStructNamedInitializer_Test1():
	c_program_text= """
		struct S
		{
			f32 a= 480.0f;
			f32 b= 3.0f + 2.0f;
		}
		fn Foo() : f32
		{
			var S s{};
			return s.a / s.b;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 96.0 )


def InClassFieldInitializer_InStructNamedInitializer_Test2():
	c_program_text= """
		struct S
		{
			[ i32, 2 ] arr[ 77, 14 ];
		}
		fn Foo() : i32
		{
			var S s{};
			return s.arr[0u] - s.arr[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 - 14 )
