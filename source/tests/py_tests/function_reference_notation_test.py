from py_tests_common import *


def ReferencePollution_TypesMismatch_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		fn DoPollution( R& mut r, i32& r ) @( 42 ); // Expected array, got integer.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReferencePollution_TypesMismatch_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var tup[] pollution;
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array, got tuple.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test2():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ i32, 2 ] pollution= zero_init;
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array of pairs, got array of ints.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test3():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ tup[ [char8, 2] ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array of pairs, got array of tuples.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test4():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [char8, 3], 2 ], 1 ] pollution[ [ "0aQ", "1_Q" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected pair of chars, got 3 chars.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test5():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [char16, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected char8, got char16.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReturnReferences_TypesMismatch_Test0():
	c_program_text= """
		fn Foo( i32& x ) : i32 & @( 66.6f ); // Expected array, got f32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 2 ) )


def ReturnReferences_TypesMismatch_Test1():
	c_program_text= """
		var tup[] return_references;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array, got tuple.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test2():
	c_program_text= """
		var [ bool, 5 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of char pairs, got array of bools
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test3():
	c_program_text= """
		var [ [ char8, 3 ], 1 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of size 2, not 3.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test4():
	c_program_text= """
		var [ [ i32, 2 ], 1 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of char8, got array of i32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReturnReInnerferences_TypesMismatch_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn Foo( i32& x ) : S @( true ); // Expected tuple, got bool.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ReturnReInnerferences_TypesMismatch_Test1():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ f32 ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected array, got f32
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ char32, 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected pair of char8, got single char32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test3():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ char32, 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected pair of char8, got single char32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test4():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ], [ [ char8, 3 ], 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Got char8 array of size 3 for second tuple element, expected size 2.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0$", "1_" ] ]; // '$' - wrong name.
		fn DoPollution( R& mut r, i32& r ) @( pollution );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0A" ]; // 'A' - wrong name. Only lower case letters are supported.
		fn Foo( i32 & x ) : i32 & @( return_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "01" ] ]; // Number as reference symbol is invalid.
		fn Foo( i32& x ) : S @( return_inner_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidParamNumber_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "^0", "1_" ] ]; // '^' - wrong name.
		fn DoPollution( R& mut r, i32& r ) @( pollution );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidParamNumber", 4 ) )


def InvalidParamNumber_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "aa" ]; // 'a' - wrong name.
		fn Foo( i32 & x ) : i32 & @( return_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidParamNumber", 4 ) )


def InvalidParamNumber_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "!_" ] ]; // '!' - wrong name/
		fn Foo( i32& x ) : S @( return_inner_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidParamNumber", 4 ) )
