from py_tests_common import *


def TupleMultipleInnerReferenceTags_Test0():
	c_program_text= """
		struct S{ i32& x; }
		struct T{ f32 &mut y; }
		static_assert( typeinfo</ S />.references_tags_count == 1s );
		static_assert( typeinfo</ T />.references_tags_count == 1s );
		static_assert( typeinfo</ tup[] />.references_tags_count == 0s );
		static_assert( typeinfo</ tup[ S ] />.references_tags_count == 1s );
		static_assert( typeinfo</ tup[ T ] />.references_tags_count == 1s );
		static_assert( typeinfo</ tup[ S, T ] />.references_tags_count == 2s );
		static_assert( typeinfo</ tup[ T, S ] />.references_tags_count == 2s );
		static_assert( typeinfo</ tup[ S, S ] />.references_tags_count == 2s );
		static_assert( typeinfo</ tup[ T, T ] />.references_tags_count == 2s );
		static_assert( typeinfo</ tup[ S, T, S, T ] />.references_tags_count == 4s );
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test1():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct R{ f32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r'x', f32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var f32 f= 0.0f;
			var R mut r{ .f= f };
			{
				var tup[ S, T ] mut t[ { .x= x }, { .y= y } ];
				DoPollution( r, t[1].y ); // Save reference inside "r".
			} // Destroy tuple, remove all inernal links.
			++x; // There are no links to "x" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test2():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var tup[ S, T ] mut t[ { .x= x }, { .y= y } ];
				DoPollution( r, t[0].x ); // Save reference inside "r".
			} // Destroy tuple, remove all inernal links.
			y *= 2.0f; // There are no links to "y" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test3():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct R{ f32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r'x', f32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var f32 f= 0.0f;
			var R mut r{ .f= f };
			{
				var tup[ S, T ] mut t[ { .x= x }, { .y= y } ];
				DoPollution( r, t[1].y ); // Save reference inside "r".
			} // Destroy tuple, remove all inernal links.
			y *= 2.0f; // Error, reference to "y" is saved inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 17 ) )


def TupleMultipleInnerReferenceTags_Test4():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 mut x= 0;
			var f32 mut y= 0.0f;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var tup[ S, T ] mut t[ { .x= x }, { .y= y } ];
				DoPollution( r, t[0].x ); // Save reference inside "r".
			} // Destroy tuple, remove all inernal links.
			--x; // Error, reference to "x" is saved inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 17 ) )


def TupleMultipleInnerReferenceTags_Test5():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution_t[ [ "0b", "1_" ] ];
		fn DoPollutionT( tup[ S, T ] &mut t'x, y', i32 &'z mut i ) @(pollution_t);
		var [ [ [char8, 2], 2 ], 1 ] pollution_r[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution_r);
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var i32 mut c= 0;
				var tup[ S, T ] mut t[ { .x= a }, { .y= b } ];
				DoPollutionT( t, c ); // Save reference to "c" in inner reference tag #1 of "t".
				DoPollutionR( r, t[1].y ); // Save reference to "c" inside "r".
			} // Error, destroyed variable "c" still have references inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 19 ) )


def TupleMultipleInnerReferenceTags_Test6():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution_t[ [ "0b", "1_" ] ];
		fn DoPollutionT( tup[ S, T ] &mut t'x, y', i32 &'z mut i ) @(pollution_t);
		var [ [ [char8, 2], 2 ], 1 ] pollution_r[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution_r);
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var i32 mut c= 0;
				var tup[ S, T ] mut t[ { .x= a }, { .y= b } ];
				DoPollutionT( t, c ); // Save reference to "c" in inner reference tag #1 of "t".
				DoPollutionR( r, t[0].x ); // Save reference to "a" inside "r".
			} // Ok, there is no references to "c" inside "r".
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test7():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		fn MakeTup( i32 &'x_tag mut x, i32 &'y_tag mut y, i32 &'z_tag mut z ) : tup[ S, T ]'x_tag, y_tag';
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			auto t= MakeTup( a, b, c );
			++c; // Ok, tuple contains references to "a" and "b", but not to "c".
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test8():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct R{ i32 &imut f; }
		fn MakeTup( i32 &'x_tag mut x, i32 &'y_tag mut y ) : tup[ S, T ]'x_tag, y_tag';
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 f= 0;
			var i32 mut a= 0;
			var R mut r{ .f= f };
			{
				var i32 mut b= 0;
				auto t= MakeTup( a, b );
				DoPollutionR( r, t[0].x ); // Create reference to "a" inside "r".
			} // Ok, "r" has reference to "a", but not to destroyed "b".
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test9():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct R{ i32 &imut f; }
		fn MakeTup( i32 &'x_tag mut x, i32 &'y_tag mut y ) : tup[ S, T ]'x_tag, y_tag';
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 f= 0;
			var i32 mut a= 0;
			var R mut r{ .f= f };
			{
				var i32 mut b= 0;
				auto t= MakeTup( a, b );
				DoPollutionR( r, t[1].y ); // Create reference to "b" inside "r".
			} // Error, "r" has reference to destroyed variable "b".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 17 ) )


def TupleMultipleInnerReferenceTags_Test10():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		fn MakeTup( i32 &'x_tag mut x, i32 &'y_tag mut y ) : tup[ S, T ]'x_tag, y_tag'
		{
			var tup[ S, T ] t[ { .x= x }, { .y= y } ];
			return t; // Return result inner references in specified in signature order.
		}
	"""
	tests_lib.build_program( c_program_text )


def TupleMultipleInnerReferenceTags_Test11():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		fn MakeTup( i32 &'x_tag mut x, i32 &'y_tag mut y ) : tup[ S, T ]'x_tag, y_tag'
		{
			var tup[ S, T ] t[ { .x= y }, { .y= x } ];
			return t; // Result inner references are in wrong order relative to specified tags in function signature.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 7 ) )


def TupleMultipleInnerReferenceTags_Test12():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ i32 & y; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s'a', i32 &'b x ) @(pollution);
		fn Foo( tup[ S, T ] &mut t )
		{
			Pollution( t[0], t[1].y ); // Perform pollution of one inner tag by another. This is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 9 ) )


def TupleMultipleInnerReferenceTags_Test13():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ i32 & y; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( T &mut t'a', i32 &'b y ) @(pollution);
		fn Foo( tup[ S, T ] &mut t )
		{
			Pollution( t[1], t[0].x ); // Perform pollution of one inner tag by another. This is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 9 ) )
