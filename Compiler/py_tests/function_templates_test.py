from py_tests_common import *


def FunctionTemplateDeclaration_Test0():
	c_program_text= """
		template</ type T />
		fn ToVoid( T& t ) : void&
		{
			return t;
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test1():
	c_program_text= """
		template</ type T, u32 size />
		fn ZeroFill( [ T, size ] &mut arr )
		{
			var u32 mut i= 0u;
			while( i < size )
			{
				arr[i]= T(0);
				++i;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateDeclaration_Test2():
	c_program_text= """
		class C
		{
			template</ type T, type U />
			fn DoNothing( T& t, U& u ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionTemplateBase_Test0():
	c_program_text= """
		template</ type T />
		fn Set42( T &mut x )
		{
			x= T(42);
		}

		fn Foo() : f32
		{
			var f32 mut x= zero_init;
			Set42(x);
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 42.0 )


def FunctionTemplateBase_Test1():
	c_program_text= """
		template</ type T, type size_type, size_type array_size />
		fn SetFirstArrayElementToArraySize( [ T, array_size ] &mut arr )
		{
			static_assert( array_size > size_type(0) );
			arr[0u]= T(array_size);
		}

		fn Foo() : i32
		{
			var [ i32, 93 ] mut arr= zero_init;
			SetFirstArrayElementToArraySize( arr );   // Must deduce all template arguments, both type and value.
			return arr[0u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 93 )
