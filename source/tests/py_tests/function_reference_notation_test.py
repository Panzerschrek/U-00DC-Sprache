from py_tests_common import *


def ReferencePollution_TypesMismatch_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		fn DoPollution( R& mut r, i32& r ) @( 42 ); // Expected array, got integer.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReferencePollution_TypesMismatch_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var tup[] pollution;
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array, got tuple.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test2():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ i32, 2 ] pollution= zero_init;
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array of pairs, got array of ints.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test3():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ tup[ [char8, 2] ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected array of pairs, got array of tuples.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test4():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [char8, 3], 2 ], 1 ] pollution[ [ "0aQ", "1_Q" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected pair of chars, got 3 chars.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReferencePollution_TypesMismatch_Test5():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [char16, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Expected char8, got char16.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReturnReferences_TypesMismatch_Test0():
	c_program_text= """
		fn Foo( i32& x ) : i32 & @( 66.6f ); // Expected array, got f32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 2 ) )


def ReturnReferences_TypesMismatch_Test1():
	c_program_text= """
		var tup[] return_references;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array, got tuple.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test2():
	c_program_text= """
		var [ bool, 5 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of char pairs, got array of bools
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test3():
	c_program_text= """
		var [ [ char8, 3 ], 1 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of size 2, not 3.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReturnReferences_TypesMismatch_Test4():
	c_program_text= """
		var [ [ i32, 2 ], 1 ] return_references= zero_init;
		fn Foo( i32& x ) : i32 & @( return_references ); // Expected array of char8, got array of i32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReturnReInnerferences_TypesMismatch_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn Foo( i32& x ) : S @( true ); // Expected tuple, got bool.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 3 ) )


def ReturnReInnerferences_TypesMismatch_Test1():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ f32 ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected array, got f32
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ char32, 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected pair of char8, got single char32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test3():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ char32, 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Expected pair of char8, got single char32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def ReturnReInnerferences_TypesMismatch_Test4():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ], [ [ char8, 3 ], 1 ] ] return_inner_references= zero_init;
		fn Foo( i32& x ) : S @( return_inner_references ); // Got char8 array of size 3 for second tuple element, expected size 2.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "TypesMismatch", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0$", "1_" ] ]; // '$' - wrong name.
		fn DoPollution( R& mut r, i32& r ) @( pollution );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0A" ]; // 'A' - wrong name. Only lower case letters are supported.
		fn Foo( i32 & x ) : i32 & @( return_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_ForFunctionReferenceNotation_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "01" ] ]; // Number as reference symbol is invalid.
		fn Foo( i32& x ) : S @( return_inner_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidParamNumber_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "^0", "1_" ] ]; // '^' - wrong name.
		fn DoPollution( R& mut r, i32& r ) @( pollution );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidParamNumber", 4 ) )


def InvalidParamNumber_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "aa" ]; // 'a' - wrong name.
		fn Foo( i32 & x ) : i32 & @( return_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidParamNumber", 4 ) )


def InvalidParamNumber_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "!_" ] ]; // '!' - wrong name.
		fn Foo( i32& x ) : S @( return_inner_references );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InvalidParamNumber", 4 ) )


def ParamNumberOutOfRange_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0a", "2_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ); // Param number is 2, but function contains only 2 params.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ParamNumberOutOfRange", 4 ) )


def ParamNumberOutOfRange_Test1():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "3_" ];
		fn Foo( i32 & x ) : i32 & @( return_references ); // Param number is 3, but function contains only 1 param.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ParamNumberOutOfRange", 3 ) )


def ParamNumberOutOfRange_Test2():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "4_" ] ];
		fn Foo( i32& x ) : S @( return_inner_references ); // Param number is 4, but function contains only 1 param.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ParamNumberOutOfRange", 4 ) )


def ReferenceTagOutOfRange_Test0():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] pollution[ [ "0b", "1_" ] ];
		fn DoPollution( R& mut r, i32& r ) @( pollution ) {} // Tag number is 1, but type contains only 1 inner reference tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ReferenceTagOutOfRange", 4 ) )


def ReferenceTagOutOfRange_Test1():
	c_program_text= """
		struct R{ i32 &imut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0c" ];
		fn Foo( R & r ) : i32 & @( return_references ) { halt; } // Tag number is 2, but type contains only 1 inner reference tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ReferenceTagOutOfRange", 4 ) )


def ReferenceTagOutOfRange_Test2():
	c_program_text= """
		struct R{ i32 &imut @("a"c8) r0; i32 &mut @("b"c8) r1; }
		struct T{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0z" ] ];
		fn Foo( R & r ) : R @( return_inner_references ) { halt; } // Tag number is 25, but type contains only 2 inner reference tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ReferenceTagOutOfRange", 5 ) )


def InnerReferenceTagCountMismatch_ForFunctionReferenceNotation_Test0():
	c_program_text= """
		struct R{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ], [ [ char8, 2 ], 0 ] ] return_inner_references[ [ "0z" ], [] ];
		fn Foo( i32& x ) : R @( return_inner_references ) { halt; } // Expected 1 tag, got 2 tags.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InnerReferenceTagCountMismatch", 4 ) )


def InnerReferenceTagCountMismatch_ForFunctionReferenceNotation_Test1():
	c_program_text= """
		struct R{ i32 &imut @("a"c8) r0; i32 &mut @("b"c8) r1; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0z" ] ];
		fn Foo( i32& x, i32 &mut y ) : R @( return_inner_references ) { halt; } // Expected 2 tags, got 1 tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InnerReferenceTagCountMismatch", 4 ) )


def InnerReferenceTagCountMismatch_ForFunctionReferenceNotation_Test2():
	c_program_text= """
		struct R{ i32& x; }
		fn generator Foo( i32& x ) : R { halt; } // Expected 1 tags, got 0 tag.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "InnerReferenceTagCountMismatch", 3 ) )


def FunctionReferenceNotationIsNormalized_Test0():
	c_program_text= """
		// Order of pollution doesn't matter.
		var [ [ [char8, 2], 2 ], 2 ] pollution_0[ [ "0a", "1_" ], [ "0a", "2_" ] ];
		var [ [ [char8, 2], 2 ], 2 ] pollution_1[ [ "0a", "2_" ], [ "0a", "1_" ] ];
		struct S{ i32& x; }
		type fn_0= ( fn( S &mut s, i32& x, i32& z ) @(pollution_0) );
		type fn_1= ( fn( S &mut s, i32& x, i32& z ) @(pollution_1) );
		static_assert( same_type</ fn_0, fn_1 /> );
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferenceNotationIsNormalized_Test1():
	c_program_text= """
		// Order of returned references doesn't matter.
		var [ [ char8, 2 ], 3 ] return_references_0[ "0_", "1a", "2b" ];
		var [ [ char8, 2 ], 3 ] return_references_1[ "2b", "0_", "1a" ];
		var [ [ char8, 2 ], 3 ] return_references_2[ "0_", "2b", "1a" ];
		var [ [ char8, 2 ], 5 ] return_references_3[ "0_", "1a", "2b", "0_", "1a" ]; // Duplicated references should be normalized-out.
		struct S{ i32& x; }
		type fn_0= ( fn( S& a, S& b, S& c ) : i32 & @(return_references_0) );
		type fn_1= ( fn( S& a, S& b, S& c ) : i32 & @(return_references_1) );
		type fn_2= ( fn( S& a, S& b, S& c ) : i32 & @(return_references_2) );
		type fn_3= ( fn( S& a, S& b, S& c ) : i32 & @(return_references_3) );
		static_assert( same_type</ fn_0, fn_1 /> );
		static_assert( same_type</ fn_0, fn_2 /> );
		static_assert( same_type</ fn_0, fn_3 /> );
		static_assert( same_type</ fn_1, fn_2 /> );
		static_assert( same_type</ fn_1, fn_3 /> );
		static_assert( same_type</ fn_2, fn_3 /> );
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferenceNotationIsNormalized_Test2():
	c_program_text= """
		// Order of returned inner references doesn't matter.
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references_0[ [ "0_", "1a", "2b" ] ];
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references_1[ [ "2b", "0_", "1a" ] ];
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references_2[ [ "0_", "2b", "1a" ] ];
		var tup[ [ [ char8, 2 ], 5 ] ] return_inner_references_3[ [ "0_", "1a", "2b", "0_", "1a" ] ]; // Duplicated references should be normalized-out.
		struct S{ i32& x; }
		type fn_0= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_0) );
		type fn_1= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_1) );
		type fn_2= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_2) );
		type fn_3= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_3) );
		static_assert( same_type</ fn_0, fn_1 /> );
		static_assert( same_type</ fn_0, fn_2 /> );
		static_assert( same_type</ fn_0, fn_3 /> );
		static_assert( same_type</ fn_1, fn_2 /> );
		static_assert( same_type</ fn_1, fn_3 /> );
		static_assert( same_type</ fn_2, fn_3 /> );
	"""
	tests_lib.build_program( c_program_text )


def DifferentReferenceNotationMeansDifferentFunctionType_Test0():
	c_program_text= """
		var [ [ [char8, 2], 2 ], 2 ] pollution_0[ [ "0a", "1_" ], [ "0a", "2_" ] ];
		var [ [ [char8, 2], 2 ], 2 ] pollution_1[ [ "0b", "1_" ], [ "0a", "2_" ] ];
		struct S{ i32& x; }
		type fn_0= ( fn( S &mut s, i32& x, i32& y ) @(pollution_0) );
		type fn_1= ( fn( S &mut s, i32& x, i32& y ) @(pollution_1) );
		static_assert( ! same_type</ fn_0, fn_1 /> );
	"""
	tests_lib.build_program( c_program_text )


def DifferentReferenceNotationMeansDifferentFunctionType_Test1():
	c_program_text= """
		var [ [ char8, 2 ], 3 ] return_references_0[ "0_", "1a", "2b" ];
		var [ [ char8, 2 ], 3 ] return_references_1[ "0_", "3a", "2b" ];
		struct S{ i32& x; }
		type fn_0= ( fn( S& a, S& b, S& c, S& d ) : i32 & @(return_references_0) );
		type fn_1= ( fn( S& a, S& b, S& c, S& d ) : i32 & @(return_references_1) );
		static_assert( !same_type</ fn_0, fn_1 /> );
	"""
	tests_lib.build_program( c_program_text )


def DifferentReferenceNotationMeansDifferentFunctionType_Test2():
	c_program_text= """
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references_0[ [ "0_", "1a", "2b" ] ];
		var tup[ [ [ char8, 2 ], 3 ] ] return_inner_references_1[ [ "0_", "1a", "2c" ] ];
		struct S{ i32& x; }
		type fn_0= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_0) );
		type fn_1= ( fn( S& a, S& b, S& c ) : S @(return_inner_references_1) );
		static_assert( !same_type</ fn_0, fn_1 /> );
	"""
	tests_lib.build_program( c_program_text )


def ExpectedConstantExpression_ForReferenceNotation_Test0():
	c_program_text= """
		fn GetReturnReferences() : [ [ char8, 2 ], 0 ];
		fn Foo(i32& x ) : i32 & @( GetReturnReferences() );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def ExpectedConstantExpression_ForReferenceNotation_Test1():
	c_program_text= """
		fn GetReturnInnerReferences() : tup[];
		fn Foo(i32& x ) : i32 @( GetReturnInnerReferences() );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def ExpectedConstantExpression_ForReferenceNotation_Test2():
	c_program_text= """
		fn GetReferencePollution() : [ [ [ char8, 2 ], 2 ], 0 ];
		fn Foo() @( GetReferencePollution() );
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HasError( errors_list, "ExpectedConstantExpression", 3 ) )


def ReferenceNotationViolatesImmutability_Test0():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn Foo( i32& x ) : i32 &mut @(return_references) // Error - result mutable reference points to an immutable arg.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 3 ) )


def ReferenceNotationViolatesImmutability_Test1():
	c_program_text= """
		var [ [ char8, 2 ], 2 ] return_references[ "0_", "1_" ];
		fn Foo( i32 &mut x, i32 &imut y ) : i32 &mut @(return_references) // Error - result mutable reference points to an immutable arg.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 3 ) )


def ReferenceNotationViolatesImmutability_Test2():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn Foo( S s ) : i32 &mut @(return_references) // Error - result mutable reference points to an immutable arg inner reference.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 4 ) )


def ReferenceNotationViolatesImmutability_Test3():
	c_program_text= """
		struct S{ i32& r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn Foo( S &mut s ) : i32 &mut @(return_references) // Error - result mutable reference points to an immutable arg inner reference.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 4 ) )


def ReferenceNotationViolatesImmutability_Test4():
	c_program_text= """
		struct S{ i32 &mut r; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn Foo( i32& x ) : S @(return_inner_references) // Error - result mutable inner reference points to an immutable arg.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 4 ) )


def ReferenceNotationViolatesImmutability_Test5():
	c_program_text= """
		struct S{ i32 &imut r; }
		struct T{ i32 &mut r; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Foo( S& s ) : T @(return_inner_references) // Error - result mutable inner reference points to an immutable arg inner reference.
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 5 ) )


def ReferenceNotationViolatesImmutability_Test6():
	c_program_text= """
		struct S{ i32 &mut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1_" ] ];
		fn Foo( S &mut s, i32& x ) @(reference_pollution) // Error - create a mutable reference inside arg "s" to immutable arg "x".
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 4 ) )


def ReferenceNotationViolatesImmutability_Test7():
	c_program_text= """
		struct S{ i32 &mut r; }
		struct T{ i32 &imut r; }
		var [ [ [ char8, 2 ], 2 ], 1 ] reference_pollution[ [ "0a", "1a" ] ];
		fn Foo( S &mut s, T t ) @(reference_pollution) // Error - create a mutable reference inside arg "s" an immutable reference of arg "t".
		{
			halt;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceNotationViolatesImmutability", 5 ) )


def ReferenceNotationInLocalScope_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn Foo()
		{
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			type fn_ptr= fn( S &mut s, i32& x ) @(pollution);
			static_assert( typeinfo</fn_ptr/>.references_pollution == pollution );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceNotationInLocalScope_Test1():
	c_program_text= """
		fn Foo()
		{
			var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
			type fn_ptr= fn( i32& x ) : i32 & @(return_references);
			static_assert( typeinfo</fn_ptr/>.return_references == return_references );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceNotationInLocalScope_Test2():
	c_program_text= """
		struct S{ i32& x; }
		fn Foo()
		{
			var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
			type fn_ptr= fn( i32& x ) : S @(return_inner_references);
			static_assert( typeinfo</fn_ptr/>.return_inner_references == return_inner_references );
		}
	"""
	tests_lib.build_program( c_program_text )
