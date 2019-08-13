from py_tests_common import *


def TupleTypeParsing_Test0():
	c_program_text= """
		fn Foo( tup(i32, f32) & arg0 );
	"""
	tests_lib.build_program( c_program_text )


def TupleTypeParsing_Test1():
	c_program_text= """
		type EmptyTuple= tup();
		type LooooongTuple= tup( i32, f32, f32, bool, [ i32, 2 ], char8, fn(), [ char16, 8 ] );
	"""
	tests_lib.build_program( c_program_text )


def TupleElementAccess_Test0():
	c_program_text= """
		fn Foo() : i32
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[0u]= 45.3f;
			t[1u]= 9;
			return i32(t[0u]) - t[ u64(7 - 6) ];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 45 - 9 )


def TupleElementAccess_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			var u32 mut x(0);
			t[x]; // index is not constant
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedConstantExpression" )
	assert( errors_list[0].file_pos.line == 6 )


def TupleElementAccess_Test2():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[ 0.5f ]; // index is not integer
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test3():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[7u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test4():
	c_program_text= """
		fn Foo()
		{
			var tup( f32, i32 ) mut t= zero_init;
			t[2u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleElementAccess_Test5():
	c_program_text= """
		fn Foo()
		{
			var tup() mut t= zero_init;
			t[0u]; // index out of bounds
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TupleIndexOutOfBounds" )
	assert( errors_list[0].file_pos.line == 5 )


def TupleFunctionArgument_Test0():
	c_program_text= """
		fn Diff( tup( f32, i32 ) t ) : i32 // Immutable value argument
		{
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) t( 52.1f, 6 );
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 - 6 )


def TupleFunctionArgument_Test1():
	c_program_text= """
		fn Diff( tup( f32, i32 ) mut t ) : i32 // Mutable value argument
		{
			t[0u]*= 3.0f;
			return i32(t[0u]) - t[1u];
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) t( 52.1f, 17 );
			return Diff(t);
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 52 * 3 - 17 )


def TupleFunctionArgument_Test2():
	c_program_text= """
		fn ZeroIt( tup( f32, i32 ) &mut t ) // Mutable reference argument
		{
			t[0u]= 0.0f;
			t[1u]= 0;
		}
		fn Foo() : i32
		{
			var tup( f32, i32 ) mut t( 6521.3f, -142 );
			ZeroIt(t);
			return i32(t[0u]) + t[1u];
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Foov" )
	assert( call_result == 0 )
