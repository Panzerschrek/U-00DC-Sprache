from py_tests_common import *


def ClassFieldReferenceNotation_Test0():
	c_program_text= """
		struct S
		{
			i32 & @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test1():
	c_program_text= """
		struct S
		{
			i32 &mut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test2():
	c_program_text= """
		struct S
		{
			i32 &imut @("a"c8) ref;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test3():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a") s;
		}
	"""
	tests_lib.build_program( c_program_text )


def ClassFieldReferenceNotation_Test4():
	c_program_text= """
		struct S{ i32& x; }
		struct T
		{
			S @("a") imut s;
		}
	"""
	tests_lib.build_program( c_program_text )


def ExpectedReferenceNotation_Test0():
	# Two reference fields - reference notation is required for both.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )


def ExpectedReferenceNotation_Test1():
	# Two reference fields - reference notation is required where it is missing.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )


def ExpectedReferenceNotation_Test2():
	# Three reference fields - reference notation is required where it is missing.
	c_program_text= """
		struct S
		{
			i32 & x;
			i32 & @("a"c8) y;
			i32 & z;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test3():
	# Two fields with references inside - reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			T y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test4():
	# Two fields with references inside - reference notation is required where it is missing.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("a") x;
			T y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test5():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			i32& y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test6():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T x;
			i32 & @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 5 ) )


def ExpectedReferenceNotation_Test7():
	# Reference field and value field with references inside. Reference notation is required for both of them.
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("a") x;
			i32& y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 6 ) )


def ExpectedReferenceNotation_Test8():
	# Notation required for reference field since base class has references inside.
	c_program_text= """
		class A polymorph { i32 & x; }
		class B : A { i32 & y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 3 ) )


def ExpectedReferenceNotation_Test9():
	# Notation required for reference field since base class has field with references inside.
	c_program_text= """
		struct S{ i32& x; }
		class A polymorph { i32 & x; }
		class B : A { S s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedReferenceNotation", 4 ) )


def InvalidInnerReferenceTagName_Test0():
	c_program_text= """
		struct S
		{
			i32& @("*"c8) y; // Non-letter symobl.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_Test1():
	c_program_text= """
		struct S
		{
			i32& @("F"c8) y; // Capital letters for now are not supported.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 4 ) )


def InvalidInnerReferenceTagName_Test2():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("$") t;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InvalidInnerReferenceTagName", 5 ) )


def InnerReferenceTagCountMismatch_Test0():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("ab") t; // Expected 1, got 2.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )


def InnerReferenceTagCountMismatch_Test1():
	c_program_text= """
		struct T{ i32 &mut x; }
		struct S
		{
			T @("") t; // Expected 1, got 0.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )


def InnerReferenceTagCountMismatch_Test2():
	c_program_text= """
		struct T
		{
			i32 &mut @("a"c8) x;
			i32 &mut @("b"c8) y;
		}
		struct S
		{
			T @("a") t; // Expected 2, got 1.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 9 ) )


def InnerReferenceTagCountMismatch_Test3():
	c_program_text= """
		struct T{}
		struct S
		{
			T @("a") t; // Expected 0, got 1.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "InnerReferenceTagCountMismatch", 5 ) )


def AutoReferenceNotationCalculation_Test0():
	# Struct without references inside has zero reference tags.
	c_program_text= """
		struct S{ i32 x; f32 y; }
		static_assert( typeinfo</S/>.references_tags_count == 0s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test1():
	# Struct with single immutable reference field - number of tags is 1.
	c_program_text= """
		struct S{ i32 &imut x; }
		static_assert( typeinfo</S/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test2():
	# Struct with single mutable reference field - number of tags is 1.
	c_program_text= """
		struct S{ i32 &mut x; }
		static_assert( typeinfo</S/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test3():
	# Struct with single field containg reference - number of tags is 1.
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ i32 x; S s; f32 y; tup[ bool, char8 ] t; }
		static_assert( typeinfo</T/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test4():
	# Struct with single field containg references - number of tags is equal to number of references inside this field.
	c_program_text= """
		struct S{ i32 &imut @("a"c8) x; i32 &imut @("b"c8) y; i32 &imut @("c"c8) z; }
		struct T{ i32 x; S s; f32 y; tup[ bool, char8 ] t; }
		static_assert( typeinfo</T/>.references_tags_count == 3s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test5():
	# Struct with single field containg references - number of tags is equal to number of references inside this field.
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ i32 x; f32 y; tup[ S, S, S, S ] t; }
		static_assert( typeinfo</T/>.references_tags_count == 4s );
	"""
	tests_lib.build_program( c_program_text )


def AutoReferenceNotationCalculation_Test6():
	# Inherit tags.
	c_program_text= """
		class A polymorph { i32 & @("a"c8) x; i32 & @("b"c8) y; }
		class B : A { f32 z; [ tup[ f32, bool ], 7 ] a; }
		static_assert( typeinfo</A/>.references_tags_count == 2s );
		static_assert( typeinfo</B/>.references_tags_count == 2s );
	"""
	tests_lib.build_program( c_program_text )


def UnusedReferenceTag_Test0():
	# "a" tag is not used.
	c_program_text= """
		struct S{ i32 & @("b"c8) x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) == 1 )
	assert( errors_list[0].error_code == "UnusedReferenceTag" )
	assert( errors_list[0].src_loc.line == 2 )


def UnusedReferenceTag_Test1():
	# "b" and "c" tags are not used.
	c_program_text= """
		struct S{ i32 & @("a"c8) x; i32 & @("d"c8) y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) == 2 )
	assert( errors_list[0].error_code == "UnusedReferenceTag" )
	assert( errors_list[0].src_loc.line == 2 )
	assert( errors_list[1].error_code == "UnusedReferenceTag" )
	assert( errors_list[1].src_loc.line == 2 )


def UnusedReferenceTag_Test2():
	# tags "a", "b", "c" are not used.
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ S @("d") s; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) == 3 )
	assert( errors_list[0].error_code == "UnusedReferenceTag" )
	assert( errors_list[0].src_loc.line == 3 )
	assert( errors_list[1].error_code == "UnusedReferenceTag" )
	assert( errors_list[1].src_loc.line == 3 )
	assert( errors_list[2].error_code == "UnusedReferenceTag" )
	assert( errors_list[2].src_loc.line == 3 )


def UnusedReferenceTag_Test3():
	# "b" tag is not used. "a" tag is inherited, "c" is specified.
	c_program_text= """
		class A polymorph { i32 &mut x; }
		class B : A { f32 &imut @("c"c8) y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) == 1 )
	assert( errors_list[0].error_code == "UnusedReferenceTag" )
	assert( errors_list[0].src_loc.line == 3 )


def MixingMutableAndImmutableReferencesInSameReferenceTag_Test0():
	# Use same tag for mutable and immutable reference fields.
	c_program_text= """
		struct S
		{
			i32 &imut @("a"c8) x;
			i32 &mut  @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MixingMutableAndImmutableReferencesInSameReferenceTag", 2 ) )


def MixingMutableAndImmutableReferencesInSameReferenceTag_Test1():
	# Use same tag for mutable field and struct with immutable reference inside.
	c_program_text= """
		struct T{ i32& x; }
		struct S
		{
			T @("a") t;
			i32 &mut @("a"c8) y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MixingMutableAndImmutableReferencesInSameReferenceTag", 3 ) )


def MixingMutableAndImmutableReferencesInSameReferenceTag_Test2():
	# Use same tag for tags of different structs with referene inside.
	c_program_text= """
		struct T{ i32& x; }
		struct W{ f32 &mut x; }
		struct S
		{
			T @("a") t;
			W @("a") w;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MixingMutableAndImmutableReferencesInSameReferenceTag", 4 ) )


def MixingMutableAndImmutableReferencesInSameReferenceTag_Test3():
	# Use same tag for inner tags of struct, which are different.
	c_program_text= """
		struct S{ i32 &mut @("a"c8) x; i32 &imut @("b"c8) y; }
		struct T
		{
			S @("aa") s;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MixingMutableAndImmutableReferencesInSameReferenceTag", 3 ) )


def MixingMutableAndImmutableReferencesInSameReferenceTag_Test4():
	# Use same tag for inner tags of tuple, which contains different tags.
	c_program_text= """
		struct T{ i32& x; }
		struct W{ f32 &mut x; }
		struct S
		{
			tup[ T, W ] @("aa") t;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "MixingMutableAndImmutableReferencesInSameReferenceTag", 4 ) )


def StructMultipleInnerReferenceTags_Test0():
	c_program_text= """
		struct S{}
		static_assert( typeinfo</S/>.references_tags_count == 0s );

		struct T{ S s; i32 x; f32 y; }
		static_assert( typeinfo</T/>.references_tags_count == 0s );

		struct R{ i32& r; }
		static_assert( typeinfo</R/>.references_tags_count == 1s );

		struct W{ i32 & @("a"c8) x; i32 & @("b"c8) y; } // Map two references to two tags.
		static_assert( typeinfo</W/>.references_tags_count == 2s );

		struct V{ i32 & @("a"c8) x; i32 & @("a"c8) y; } // Map two references to single tag.
		static_assert( typeinfo</V/>.references_tags_count == 1s );

		struct Q{ W @("ab") w; R @("c") r; }
		static_assert( typeinfo</Q/>.references_tags_count == 3s );

		struct H{ Q @("aaa") q; } // Map 3 references to single tag.
		static_assert( typeinfo</H/>.references_tags_count == 1s );
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test1():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct W{ S @("a") s; T @("b") t; }
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
				var W mut w{ .s{ .x= x }, .t{ .y= y } };
				DoPollution( r, w.t.y ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			++x; // There are no links to "x" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test2():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct W{ S @("a") s; T @("b") t; }
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
				var W mut w{ .s{ .x= x }, .t{ .y= y } };
				DoPollution( r, w.s.x ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			y *= 2.0f; // There are no links to "y" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test3():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct W{ S @("a") s; T @("b") t; }
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
				var W mut w{ .s{ .x= x }, .t{ .y= y } };
				DoPollution( r, w.t.y ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			y *= 2.0f; // Error, reference to "y" is saved inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 18 ) )


def StructMultipleInnerReferenceTags_Test4():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct W{ S @("a") s; T @("b") t; }
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
				var W mut w{ .s{ .x= x }, .t{ .y= y } };
				DoPollution( r, w.s.x ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			--x; // Error, reference to "x" is saved inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 18 ) )


def StructMultipleInnerReferenceTags_Test5():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution_w[ [ "0b", "1_" ] ];
		fn DoPollutionW( W &mut w'x, y', i32 &'z mut i ) @(pollution_w);
		var [ [ [char8, 2], 2 ], 1 ] pollution_r[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution_r);
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var i32 mut c= 0;
				var W mut w{ .s{ .x= a }, .t{ .y= b } };
				DoPollutionW( w, c ); // Save reference to "c" in inner reference tag #1 of "w".
				DoPollutionR( r, w.t.y ); // Save reference to "c" inside "r".
			} // Error, destroyed variable "c" still have references inside "r".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 20 ) )


def StructMultipleInnerReferenceTags_Test6():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		struct R{ i32 &imut f; }
		var [ [ [char8, 2], 2 ], 1 ] pollution_w[ [ "0b", "1_" ] ];
		fn DoPollutionW( W &mut t'x, y', i32 &'z mut i ) @(pollution_w);
		var [ [ [char8, 2], 2 ], 1 ] pollution_r[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution_r);
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0;
			var i32 f= 0;
			var R mut r{ .f= f };
			{
				var i32 mut c= 0;
				var W mut w{ .s{ .x= a }, .t{ .y= b } };
				DoPollutionW( w, c ); // Save reference to "c" in inner reference tag #1 of "w".
				DoPollutionR( r, w.s.x ); // Save reference to "a" inside "r".
			} // Ok, there is no references to "c" inside "r".
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test7():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		fn MakeW( i32 &'x_tag mut x, i32 &'y_tag mut y, i32 &'z_tag mut z ) : W'x_tag, y_tag';
		fn Foo()
		{
			var i32 mut a= 0, mut b= 0, mut c= 0;
			auto W= MakeW( a, b, c );
			++c; // Ok, W contains references to "a" and "b", but not to "c".
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test8():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		struct R{ i32 &imut f; }
		fn MakeW( i32 &'x_tag mut x, i32 &'y_tag mut y ) : W'x_tag, y_tag';
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 f= 0;
			var i32 mut a= 0;
			var R mut r{ .f= f };
			{
				var i32 mut b= 0;
				auto w= MakeW( a, b );
				DoPollutionR( r, w.s.x ); // Create reference to "a" inside "r".
			} // Ok, "r" has reference to "a", but not to destroyed "b".
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test9():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		struct R{ i32 &imut f; }
		fn MakeW( i32 &'x_tag mut x, i32 &'y_tag mut y ) : W'x_tag, y_tag';
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollutionR( R& mut r'x', i32 &'y f ) @(pollution);
		fn Foo()
		{
			var i32 f= 0;
			var i32 mut a= 0;
			var R mut r{ .f= f };
			{
				var i32 mut b= 0;
				auto w= MakeW( a, b );
				DoPollutionR( r, w.t.y ); // Create reference to "b" inside "r".
			} // Error, "r" has reference to destroyed variable "b".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "DestroyedVariableStillHaveReferences", 18 ) )


def StructMultipleInnerReferenceTags_Test10():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		fn MakeW( i32 &'x_tag mut x, i32 &'y_tag mut y ) : W'x_tag, y_tag'
		{
			var W w{ .s{ .x= x }, .t{ .y= y } };
			return w; // Return result inner references in specified in signature order.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test11():
	c_program_text= """
		struct S{ i32 &mut x; }
		struct T{ i32 &mut y; }
		struct W{ S @("a") s; T @("b") t; }
		fn MakeW( i32 &'x_tag mut x, i32 &'y_tag mut y ) : W'x_tag, y_tag'
		{
			var W w{ .s{ .x= y }, .t{ .y= x } };
			return w; // Result inner references are in wrong order relative to specified tags in function signature.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReturningUnallowedReference", 8 ) )


def StructMultipleInnerReferenceTags_Test12():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ i32 & y; }
		struct W{ S @("a") s; T @("b") t; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s'a', i32 &'b x ) @(pollution);
		fn Foo( W &mut w )
		{
			Pollution( w.s, w.t.y ); // Perform pollution of one inner tag by another. This is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 10 ) )


def TupleMultipleInnerReferenceTags_Test13():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ i32 & y; }
		struct W{ S @("a") s; T @("b") t; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( T &mut t'a', i32 &'b y ) @(pollution);
		fn Foo( W &mut w )
		{
			Pollution( w.t, w.s.x ); // Perform pollution of one inner tag by another. This is not allowed.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "UnallowedReferencePollution", 10 ) )


def StructMultipleInnerReferenceTags_Test14():
	c_program_text= """
		struct S{ i32 &imut x; }
		struct T{ f32 &mut  y; }
		struct V{ S @("a") s; T @("b") t; }
		struct W{ V @("ba") v; } // Reverse order of reference tags.
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
				var W mut w{ .v{ .s{ .x= x }, .t{ .y= y } } };
				DoPollution( r, w.v.s.x ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			y *= 2.0f; // There are no links to "y" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test15():
	c_program_text= """
		struct W{ i32 &imut @("a"c8) x; f32 &mut @("b"c8) y; }
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
				var W mut w{ .x= x, .y= y };
				DoPollution( r, w.x ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			y *= 2.0f; // There are no links to "y" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructMultipleInnerReferenceTags_Test16():
	c_program_text= """
		struct W{ i32 &imut @("a"c8) x; f32 &mut @("b"c8) y; }
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
				var W mut w{ .x= x, .y= y };
				DoPollution( r, w.y ); // Save reference inside "r".
			} // Destroy W struct, remove all inernal links.
			++x; // There are no links to "x" left, so, we can mutate it.
		}
	"""
	tests_lib.build_program( c_program_text )


def TypesMismatch_ForFieldReferenceNotation_Test0():
	c_program_text= """
		struct S{ i32 & @(42) x; } // Expected char8, given int.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForFieldReferenceNotation_Test1():
	c_program_text= """
		struct S{ i32 & @("a") x; } // Expected char8, given array of char8.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForFieldReferenceNotation_Test2():
	c_program_text= """
		struct S{ i32 & @("a"c16) x; } // Expected char8, given char16.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 2 ) )


def TypesMismatch_ForFieldReferenceNotation_Test3():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ S @(0.5f) s; } // Expected array of char8, given f32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def TypesMismatch_ForFieldReferenceNotation_Test4():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ S @("a"c8) s; }  // Expected array of char8, given char8.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def TypesMismatch_ForFieldReferenceNotation_Test5():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ S @("a"u32) s; }  // Expected array of char8, given array of char32.
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "TypesMismatch", 3 ) )


def ExpectedConstantExpression_ForFieldReferenceNotation_Test0():
	c_program_text= """
		struct S{ i32 & @( Foo() ) x; } // Call to non-constexpr function.
		fn Foo() : char8;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedConstantExpression", 2 ) )


def ExpectedConstantExpression_ForFieldReferenceNotation_Test1():
	c_program_text= """
		struct S{ i32 & x; }
		struct T{ S @( Foo() ) s; } // Call to non-constexpr function.
		fn Foo() : [ char8, 1 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ExpectedConstantExpression", 3 ) )
