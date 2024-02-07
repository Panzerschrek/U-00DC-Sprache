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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 5 )


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
	assert( errors_list[0].src_loc.line == 4 )


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
	assert( errors_list[0].src_loc.line == 4 )
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
	assert( errors_list[0].src_loc.line == 4 )
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
	assert( errors_list[0].src_loc.line == 6 )
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
	assert( errors_list[0].src_loc.line == 6 )
	assert( errors_list[0].text.find( "\"y\"" ) != -1 )


def TernaryOperator_TemporariesAreDestroyed_Test0():
	c_program_text= """
		fn Foo( bool b ) : i32
		{
			// Temporary values are produced in each branch of "select" expression.
			// All temporaries, produced in each branch of "select" operator are destroyed,
			// because there is no proper way to destroy them after branching - it is not clear what was actually created because of brancing.
			// Because of that  it is not possible to take a reference to temporary value, using "select" branch.
			// Alternatives are using values instead of references or using variables with longer lifetimes (stack variables, function params, global variables).
			return select( b
				? cast_imut(123) // Cast value to reference in order to "select" result have kind "ReferenceImut".
				: cast_imut(456) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 10 ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 11 ) )


def TernaryOperator_TemporariesAreDestroyed_Test1():
	c_program_text= """
		fn Foo( bool b ) : i32
		{
			var i32 x= 456;
			return select( b
				? cast_imut(123) // Cast value to reference in order to "select" result have kind "ReferenceImut".
				: x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 1 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 6 ) )


def TernaryOperator_TemporariesAreDestroyed_Test2():
	c_program_text= """
		fn Foo( bool b ) : i32
		{
			var i32 x= 123;
			return select( b
				? x
				: cast_imut(456) ); // Cast value to reference in order to "select" result have kind "ReferenceImut".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 1 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 7 ) )


def TernaryOperator_TemporariesAreDestroyed_Test3():
	c_program_text= """
		fn Pass( i32& x ) : i32& { return x; }
		fn Foo( bool b )
		{
			auto& r= select(
				b
				? Pass(5)
				: Pass(7) ); // Both branches have value_type= ValueType::ConstReference, so, result will be const reference. But, referenced variables will be destroyed after branches evaluation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 7 ) )
	assert( HasError( errors_list, "DestroyedVariableStillHasReferences", 8 ) )


def VariablesStateMerge_ForTernaryOperator_Test0():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 mut x= 0;
			auto moved= select( b ? x : move(x) );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def VariablesStateMerge_ForTernaryOperator_Test1():
	c_program_text= """
		fn Foo( bool b )
		{
			var i32 mut x= 0;
			auto moved= select( b ? move(x) : x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ConditionalMove" )
	assert( errors_list[0].src_loc.line == 5 )


def VariablesStateMerge_ForTernaryOperator_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn FakePollution( S &mut s, i32& i ) @(pollution) : i32 { return i; }
		fn Foo( bool b )
		{
			var i32 mut x= 7, mut y= 5, t= 0;
			var S mut s{ .x= t };
			auto z= select( b ? FakePollution( s, x ) : y );
			++x; // Error, 'x' already have reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def VariablesStateMerge_ForTernaryOperator_Test3():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn FakePollution( S &mut s, i32& i ) @(pollution) : i32 { return i; }
		fn Foo( bool b )
		{
			var i32 mut x= 7, mut y= 5, t= 0;
			var S mut s{ .x= t };
			auto z= select( b ? x : FakePollution( s, y ) );
			++y; // Error, 'y' already have reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def VariablesStateMerge_ForTernaryOperator_Test4():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn FakePollution( S &mut s, i32& mut i ) @(pollution) : i32 { return 0; }
		fn Foo( bool b )
		{
			var i32 mut x= 0, mut t= 0, mut u= 0;
			var S mut s0{ .x= t }, mut s1{ .x= u };
			auto z= select( b ? FakePollution( s0, x ) : FakePollution( s1, x ) ); // Create mutable references to "x" in different variables. It is not actually error now.
		}
	"""
	tests_lib.build_program( c_program_text )


def TernaryOperator_SavesInnerReferences_Test0():
	c_program_text= """
		struct S
		{
			i32 &mut x;
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn GetS( i32 & mut x ) : S @(return_inner_references)
		{
			var S mut s{ .x= x };
			return move(s);
		}
		fn Foo( bool b )
		{
			var i32 mut x= 0, mut y= 0;
			auto res= select( b ? GetS(x) : GetS(y) );
			++x; // Error, "x" have reference inside "res"
			++y; // Error, "y" have reference inside "res"
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 16 )
	assert( errors_list[1].error_code == "ReferenceProtectionError" )
	assert( errors_list[1].src_loc.line == 17 )
