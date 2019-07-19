from py_tests_common import *


def CommonValueType_Test0():
	c_program_text= """
		fn Foo( bool b )
		{
			auto mut x= 0;
			++ select( b ? x : 10 ); // first argument - "Reference", second - "Value". Common value type will be "Value".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 5 )


def CommonValueType_Test1():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 mut x= 0, imut y= 0;
			++ select( b ? x : y ); // first argument - "Reference", second - "ConstReference". Common value type will be "ConstReference".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 5 )


def CommonValueType_Test2():
	c_program_text= """
		fn Foo( bool b )
		{
			++ select( b ? 5 : 7 ); // value type of both branches is "Value", result value type will be "Value".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedReferenceValue" )
	assert( errors_list[0].file_pos.line == 4 )


def TernaryOperator_TypesMismatch_Test0():
	c_program_text= """
		fn Foo( i32 b )
		{
			select( b ? 1 : 2 ); // "Expected bool, got i32"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 4 )
	assert( errors_list[0].text.find( "bool" ) != -1 )


def TernaryOperator_TypesMismatch_Test1():
	c_program_text= """
		fn Foo( bool b )
		{
			select( b ? 1.0f : 2u );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].file_pos.line == 4 )
	assert( errors_list[0].text.find( "f32" ) != -1 )
	assert( errors_list[0].text.find( "u32" ) != -1 )


def TernaryOperator_ReferenceProtectionError_Test0():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 mut x= 0, mut y= 0;
			auto &mut r= select( b ? x : y );
			auto& r2= x; // error, reference to 'x' already exists in 'r'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 6 )
	assert( errors_list[0].text.find( "\"x\"" ) != -1 )


def TernaryOperator_ReferenceProtectionError_Test1():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 mut x= 0, mut y= 0;
			auto &mut r= select( b ? x : y );
			auto& r2= y; // error, reference to 'y' already exists in 'r'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].file_pos.line == 6 )
	assert( errors_list[0].text.find( "\"y\"" ) != -1 )


def DestroyedVariableStillHaveReference_Test0():
	c_program_text= """
		fn Pass( i32& x ) : i32& { return x; }
		fn Foo( bool b )
		{
			auto& r= select( b ? Pass(5) : Pass(7) ); // Both branches have value_type= ValueType::ConstReference, so, result will be const reference. But, referenced variables will be destroyed after branches evaluation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( errors_list[0].error_code == "DestroyedVariableStillHaveReferences" )
	assert( errors_list[0].file_pos.line == 5 )
	assert( errors_list[1].error_code == "DestroyedVariableStillHaveReferences" )
	assert( errors_list[1].file_pos.line == 5 )
