from py_tests_common import *

def PointerTypeDeclaration_Test0():
	c_program_text= """
		type IntPtr = $(i32);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test1():
	c_program_text= """
		type IntPtrPtr = $($(i32));
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test2():
	c_program_text= """
		type IntArray4Ptr = $([i32, 4]);
	"""
	tests_lib.build_program( c_program_text )


def PointerTypeDeclaration_Test3():
	c_program_text= """
		type EmptyTupPtr = $(tup[]);
	"""
	tests_lib.build_program( c_program_text )


def RawPointerTypeUsage_Test0():
	c_program_text= """
		fn Foo( $(i32) x, $($(f32)) y ){}
	"""
	tests_lib.build_program( c_program_text )


def RawPointerInitializers_Test0():
	c_program_text= """
		fn Foo() : size_type
		{
			var $(i32) z= zero_init;
			unsafe{  return cast_ref_unsafe</size_type/>(z);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )


def ReferenceToPointerOperator_Test0():
	c_program_text= """
		fn Foo() : size_type
		{
			var i32 x= 0;
			var $(i32) x_ptr= $<(x);
			unsafe{  return cast_ref_unsafe</size_type/>(x_ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result != 0 ) # variable should have non-zero address


def ReferenceToPointerOperator_Test1():
	c_program_text= """
		fn Foo() : size_type
		{
			var i32 x= 0;
			var $(i32) x_ptr= $<(x);
			var $($(i32)) x_ptr_ptr= $<(x_ptr);
			unsafe{  return cast_ref_unsafe</size_type/>(x_ptr_ptr);  }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result != 0 ) # variable should have non-zero address


def PointerToReferenceOperator_Test0():
	c_program_text= """
		fn Foo() : u32
		{
			var u32 mut x= 0u;
			var $(u32) x_ptr= $<(x);
			unsafe{  $>(x_ptr)= 678u;  } // Write value, using pointer.
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 678 )


def PointerToReferenceOperator_Test1():
	c_program_text= """
		fn Foo() : f64
		{
			auto mut f= -1.0;
			var $(f64) f_ptr= $<(f);
			auto f_ptr_ptr= $<(f_ptr);
			unsafe{  $>($>(f_ptr_ptr))= 37.5;  }
			return f;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 37.5 )


def PointerToReferenceOperator_Test2():
	c_program_text= """
		fn Foo() : i32
		{
			auto x= 73472;
			auto ptr= $<(x);
			unsafe{  return $>(ptr);  } // Read value, using pointer.
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 73472 )


def PointerToReferenceOperator_Test3():
	c_program_text= """
		fn Foo() : f32
		{
			auto x= 654.5f;
			unsafe{  return $>($<(x));  } // Directly use pointer for conversion to reference
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 654.5 )
