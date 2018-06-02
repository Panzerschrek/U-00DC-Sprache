from py_tests_common import *


def FunctionTypeDeclaration_Test0():
	c_program_text= """
		// As global variable
		var fn( i32 x, i32 y ) constexpr pointer_to_some_function= zero_init;
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test1():
	c_program_text= """
		// As local variable
		fn Foo()
		{
			var fn( i32 x, i32 y ) : i32 pointer_to_some_function= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test2():
	c_program_text= """
		// As local variable
		fn Foo()
		{
			// With brackets
			var ( fn( i32 x, i32 y ) : i32& ) pointer_to_some_function= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test3():
	c_program_text= """
		// In typedef
		type BinaryIntFunction= fn( i32 a, i32 b ) : i32;
		fn Foo( BinaryIntFunction a )
		{}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test4():
	c_program_text= """
		// As function argument. Also unsafe
		fn Foo( fn() unsafe arg )
		{}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTypeDeclaration_Test5():
	c_program_text= """
		// As field. Also pollution list
		struct F{ i32& r; }
		struct S
		{
			( fn( F& mut f'a', i32&'b x ) ' a <- imut b '  ) some_function;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test0():
	c_program_text= """
		// Zero initializer
		fn Foo()
		{
			var ( fn() ) foo= zero_init;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test1():
	c_program_text= """
		// Expression initializer and single function
		fn Foo()
		{
			var ( fn() ) foo= Foo;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test2():
	c_program_text= """
		// Expression initializer and multiple functions.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) int_func= a;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test3():
	c_program_text= """
		// Constructor initializer and multiple functions.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) int_func( a );
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test4():
	c_program_text= """
		// Initialize, using other function pointer.
		fn a( i32 x ){}
		fn a( f32 x ){}
		fn Foo()
		{
			var ( fn( i32 x ) ) mut int_func_0( a );
			var ( fn( i32 x ) ) int_func_1= int_func_0;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionPoinerInitialization_Test5():
	c_program_text= """
		// Initialize function pointer, using method.
		struct S
		{
			i32 x;
			fn GetX( this ) : i32 { return x; }
		}
		fn Foo()
		{
			var ( fn( S& s ) : i32 ) getter= S::GetX;
		}
	"""
	tests_lib.build_program( c_program_text )
