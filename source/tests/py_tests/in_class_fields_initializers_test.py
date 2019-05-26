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


def InClassFieldInitializer_InConstructorInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 3258;
			fn constructor()
			// In default initializer list initializer for 'x' called
			{}
		}
		fn Foo() : i32
		{
			return S().x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3258 )


def InClassFieldInitializer_InConstructorInitializer_Test1():
	c_program_text= """
		struct S
		{
			i32 x= 89;
			i32 y;
			fn constructor()
			( y= x * 5 ) // 'x' initialized before 'y'
			{}
		}
		fn Foo() : i32
		{
			var S s;
			return s.y - s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 89 * 5 - 89 )


def InClassFieldInitializer_InDefaultConstructor_Test0():
	c_program_text= """
		struct S
		{
			i32 x= -1;
			f32 y= -99.0f;
		}
		fn Foo() : i32
		{
			var S s; // Default constructor generated, because struct have initializer for all fields.
			return i32(f32(s.x) * s.y);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 99 )


def InClassFieldInitializer_InDefaultConstructor_Test1():
	c_program_text= """
		struct S
		{
			i32 x= 0;
		}
		struct T
		{
			S s; // Have default constructor.
			bool b= false;
		}
		fn Foo()
		{
			var T t; // Default constructor generated.
			halt if( t.s.x != 0 );
			halt if( t.b );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test0():
	c_program_text= """
		struct S
		{
			i32 x= 95;
		}
		fn Foo() : i32
		{
			var S s{ .x= 77 };
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 77 )


def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test1():
	c_program_text= """
		struct S
		{
			[ i64, 6 ] arr= zero_init;
		}
		fn Foo() : i64
		{
			var S s{ .arr[ 4i64, 8i64, 15i64, 16i64, 23i64, 42i64 ] };
			return s.arr[0u] + s.arr[1u] * s.arr[2u] / s.arr[3u] + s.arr[4u] - s.arr[5u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 4 + int(8 * 15 / 16) + 23 - 42 )



def ImplicitInitializerUsedInsteadOf_InClassFieldInitializer_Test2():
	c_program_text= """
		struct S
		{
			i32 x= 0;
			fn constructor( i32 in_x )
			( x(in_x) )
			{}
		}
		fn Foo() : i32
		{
			var S s( 33369 );
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 33369 )
