from py_tests_common import *


def NamespaceMixinDeclaration_Test0():
	c_program_text= """
		mixin( "fn Foo() : i32 { return 665533; }" );
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 665533 )


def NamespaceMixinDeclaration_Test1():
	c_program_text= """
		namespace Some
		{
			mixin( "fn Foo() : f32 { return 3.25f; }" );
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_ZN4Some3FooEv" ) == 3.25 )


def ClassMixinDeclaration_Test0():
	c_program_text= """
		struct S
		{
			mixin( "i32 x;" ); // Add a field via mixin.
		}
		fn Foo() : i32
		{
			var S s{ .x= 76567 };
			return s.x;
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 76567 )


def ClassMixinDeclaration_Test1():
	c_program_text= """
		struct S
		{
			auto x_field= "u64 x;";
			auto x_method= "fn GetSquaredX( this ) : u64 { return x * x; }";
			mixin( x_field + "\\n\\t" + x_method ); // Add a field and a method in mixin.
		}
		fn Foo() : u64
		{
			var S s{ .x(543) };
			return s.GetSquaredX();
		}
	"""
	tests_lib.build_program( c_program_text )
	assert( tests_lib.run_function( "_Z3Foov" ) == 543 * 543 )
