from py_tests_common import *


def GlobalsLoop_ForFunctionWithAutoReturnType_Test0():
	c_program_text= """
		fn Foo() : auto
		{
			Foo(); // Error, acces to "Foo" forbiden, while compiling "Foo"
			return 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )



def GlobalsLoop_ForFunctionWithAutoReturnType_Test1():
	c_program_text= """
		fn Foo( i32 x ) : i32 { return x; }
		fn Foo() : auto
		{
			Foo( 0 ); // Error, acces to "Foo" forbiden, while compiling "Foo". We access all functions set, not single function Foo(i32).
			return 0;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "GlobalsLoopDetected" )
	assert( errors_list[0].src_loc.line == 2 )


def ExpectedBodyForAutoFunction_Test0():
	c_program_text= """
		fn Foo() : auto;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedBodyForAutoFunction" )
	assert( errors_list[0].src_loc.line == 2 )


def ExpectedBodyForAutoFunction_Test1():
	c_program_text= """
		fn Foo( i32 x ) : auto&;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ExpectedBodyForAutoFunction" )
	assert( errors_list[0].src_loc.line == 2 )


def TypesMismtach_ForAutoReturnValue_Test0():
	c_program_text= """
		fn Foo( bool b ) : auto
		{
			if( b )
			{ return 1; } // return type deduced to i32
			return -1.0; // return type deduced to f64
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def TypesMismtach_ForAutoReturnValue_Test1():
	c_program_text= """
		fn Foo( bool b ) : auto
		{
			if( b )
			{ return; } // return type deduced to void
			return -1.0; // return type deduced to f64
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "TypesMismatch" )
	assert( errors_list[0].src_loc.line == 6 )


def ExpectedReferenceValue_ForAutoReturnValue_Test0():
	c_program_text= """
		fn Foo( bool b ) : auto&
		{
			return; // Error, expected reference
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 1 )
	assert( errors_list[1].error_code == "ExpectedReferenceValue" )
	assert( errors_list[1].src_loc.line == 4 )


def ConstexprFunctionContainsUnallowedOperations_ForAutoReturnFunction_Test0():
	c_program_text= """
		fn constexpr Foo() : auto
		{
			unsafe {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ConstexprFunctionContainsUnallowedOperations", 2 ) )


def ReferenceNotationForAutoFunction_Test0():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn Foo( i32& x ) : auto& @(return_references)
		{
			return x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForAutoFunction", 3 ) )


def ReferenceNotationForAutoFunction_Test1():
	c_program_text= """
		struct S{ i32& r; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn Foo( i32& x ) : auto @(return_inner_references)
		{
			var S s{ .r= x };
			return s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForAutoFunction", 4 ) )


def ReferenceNotationForAutoFunction_Test2():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Bar( S &mut s, i32& x ) @(pollution) : auto
		{
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationForAutoFunction", 4 ) )


def AutoForFunctionTypeReturnType_Test0():
	c_program_text= """
		type BrokenPtr= fn() : auto;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForFunctionTypeReturnType", 2 ) )


def AutoForFunctionTypeReturnType_Test1():
	c_program_text= """
		type BrokenPtr= fn() : auto&;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForFunctionTypeReturnType", 2 ) )


def AutoForFunctionTypeReturnType_Test2():
	c_program_text= """
		type BrokenPtr= fn() : [ auto, 4 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "NameNotFound", 2 ) )


def AutoForSpecialMethod_Test0():
	c_program_text= """
		struct S
		{
			fn constructor() : auto // default constructor
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test1():
	c_program_text= """
		struct S
		{
			fn constructor( i32 x ) : auto // custom constructor
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test2():
	c_program_text= """
		struct S
		{
			fn destructor() : auto
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test3():
	c_program_text= """
		struct S
		{
			op=( mut this, S& other ) : auto // auto for copy-assignment operator
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test4():
	c_program_text= """
		struct S
		{
			op=( mut this, i32 x ) : auto // auto for custom assignment operator
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test5():
	c_program_text= """
		struct S
		{
			op==( S& a, S& b ) : auto // auto for equality compare operator
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForSpecialMethod_Test6():
	c_program_text= """
		struct S
		{
			op==( S& a, f32 x ) : auto // auto for custom equality compare operator
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForSpecialMethod", 4 ) )


def AutoForVirtualMethod_Test0():
	c_program_text= """
		class C polymorph
		{
			fn virtual Foo(this) : auto
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForVirtualMethod", 4 ) )


def AutoForVirtualMethod_Test1():
	c_program_text= """
		class C interface
		{
			op virtual ()(this) : auto
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "AutoForVirtualMethod", 4 ) )
