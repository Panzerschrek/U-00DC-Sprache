from py_tests_common import *


def OrederIndependentFunctions_Test0():
	c_program_text= """
		fn Baz() : i32 { return 81; }
		fn Foo() : i32
		{
			return Baz() * Bar();  // Functions "Bar" and "Baz" visible here and can be called.
		}
		fn Bar() : i32 { return 52414; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52414 * 81 )


def OrederIndependentClasses_Test0():
	c_program_text= """
		fn Foo()
		{
			var S s;
		}

		struct T{}
		struct S{ F f; T t; }  // Here used classes abowe and below.
		struct F{}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )


def OrederIndependent_OutOfLineFunction_Test0():
	c_program_text= """
		fn Bar() : i32;
		fn Foo() : i32  { return Bar(); }
		fn Bar() : i32  { return 88524; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 88524 )


def OrederIndependent_OutOfLineFunction_Test1():
	c_program_text= """
		fn Bar() : i32  { return 665235; }
		fn Foo() : i32  { return Bar(); }
		fn Bar() : i32; // ok, prototype after body

	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 665235 )


def OrederIndependent_OutOfLineFunction_Test2():
	c_program_text= """
		fn Foo() : i32  { return N::Bar(); }
		namespace N
		{
			fn Bar() : i32;
		}
		fn N::Bar() : i32 { return 5632478; }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5632478 )


def OrederIndependent_OutOfLineFunction_Test3():
	c_program_text= """
		fn Foo() : i32  { return N::Bar(); }
		namespace N
		{
			fn Bar() : i32;
			fn Bar() : i32  { return 5623222; }
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 5623222 )


def OrederIndependent_Enums_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			return i32(E::C);
		}
		enum E{ A, B, C, D }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 2 )


def OrederIndependent_Enums_Test1():
	c_program_text= """
		fn Foo() : i32
		{
			var SS ss= SS::D;
			return i32(ss);
		}
		enum SS{ A, B, C, D }
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 3 )


def OrderIndependent_Typedef_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var I r= 51245;
			return r;
		}
		type I= i32;
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 51245 )


def OrderIndependent_Typedef_Test1():
	c_program_text= """
		type A= RRR;
		fn Foo() : A
		{
			return 653524;
		}
		type FFF= i32;
		type RRR= FFF;
		type UnusedType= [ bool, 1024 ];
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 653524 )
