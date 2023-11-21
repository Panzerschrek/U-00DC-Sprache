from py_tests_common import *


def SimpleProgramTest():
	c_program_text= """
		fn Add( i32 a, i32 b ) : i32
		{
			return a + b;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Addii", 95, -5 )
	assert( call_result == 95 - 5 )


def LocalVariableTest():
	c_program_text= """
		fn Sub( i32 a, i32 b ) : i32
		{
			var i32 mut x= zero_init;
			x= a - b;
			return x;
		}
	"""
	tests_lib.build_program( c_program_text )
	call_result= tests_lib.run_function( "_Z3Subii", 654, 112 )
	assert( call_result == 654 - 112 )


def CreateMutableReferenceToVariableWithImmutableReference_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= zero_init;
			auto& imut ri= x;
			auto& mut  rm= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToVariableWithImmutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= zero_init;
			var i32 &imut ri= x;
			var i32 &mut  rm= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToVariableWithMutableReference():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= x;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def CreateMutableReferenceToAnotherMutableReference_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0; // ok
		}
	"""
	tests_lib.build_program( c_program_text )


def CreateMutableReferenceToAnotherMutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0; // ok
			var i32 &mut r2= r0; // Not ok
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def AccessingVariableThatHaveMutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r0= x;
			var i32 &mut r1= r0;
			var i32& r2= r0; // error, accessing 'r0', that have reference 'r1'
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def AccessingVariableThatHaveMutableReference_Test2():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			var i32 &mut r= x;
			var i32 x_copy= r; //ok, accessing end reference
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferencePass_Test0():
	c_program_text= """
		fn Pass( i32& x ) : i32& { return x; }
		fn Foo()
		{
			auto mut x= 0;
			auto &imut ri= Pass(x);
			auto &mut rm= x; // Error, 'x' have reference already.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def FunctionReferencePass_Test1():
	c_program_text= """
		var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
		fn PassFirst( i32& x, i32& y ) : i32& @(return_references) { return x; }
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			auto &imut ri= PassFirst( x, y );
			auto &mut rm= y; // Ok, 'y' have no references.
		}
	"""
	tests_lib.build_program( c_program_text )


def FunctionReferencePass_Test2():
	c_program_text= """
		fn Pass( i32&mut x ) : i32&mut { return x; }
		fn Foo()
		{
			auto mut x= 0;
			auto &mut r0= Pass(x);
			auto& x_ref= x; // Error, taking reference to 'x', which have mutable rerefernce.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32 &imut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			++x; // Error, 'x' have reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReferenceInsideStruct_Test1():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= x;
			var S s{ .r= x }; // Error, creating mutable reference to variable, that have reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 7 )


def ReturnReferenceInsideStruct_Test1():
	c_program_text= """
		struct S{ i32 &imut r; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn GetS( i32 & imut x ) : S @(return_inner_references)
		{
			var S s{ .r= x };
			return s;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			auto &imut r= GetS(x).r;
			++x; // Error, reference to 'x' exists inside 'r'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 13 )


def ReturnReferenceFromStruct_Test0():
	c_program_text= """
		struct S{ i32 &mut r; }
		var [ [ char8, 2 ], 1 ] return_references[ "0a" ];
		fn GetR( S& s ) : i32 &mut @(return_references)
		{
			return s.r;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			var i32 &mut x_ref= GetR( s );
			++s.r; // Error, reference to 'x' exists inside 'r'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 13 )


def ReturnReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32& x; }
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn GetS( i32& x ) : S @(return_inner_references)
		{
			var S s{ .x= x };
			return s;
		}
		fn Foo()
		{
			var i32 mut x= 0;
			auto s= GetS(x);
			++x; // Error, reference to 'x' exists inside 's'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 13 )


def PollutionTest0():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			Pollution( s, y );
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PollutionTest1():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn Pollution( S &mut s, i32& x ) @(pollution) {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s{ .x= x };
			auto& mut s_ref= s;
			Pollution( s_ref, y );
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 11 )


def PollutionTest2():
	c_program_text= """
		struct S{ i32& x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
		fn Pollution( S &mut a, S& b ) @(pollution) {}
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut a{ .x= x };
			{
				var S mut b{ .x= y };
				Pollution( a, b ); // Now, 'a', contains reference to 'y'
			}
			++y;
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 13 )


def ReferenceFieldAccess_Test0():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto& s_ref= s;
			auto& x_ref= s_ref.x; // Accessing reference field of struct 's', using reference to struct 's'
			++s.x; // Error, reference to 'x' inside 's' is not unique.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 9 )


def PassMutableReferenceTwoTimes_Tes0():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& s, i32 &mut i ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s.x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S s, i32 &mut i ) {}
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s.x );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes2():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes3():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S a, S b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes4():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes5():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S a, S b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 8 )


def PassMutableReferenceTwoTimes_Tes6():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S& a, S& b ) { ++a.x; ++b.x; }
		fn Foo( S& s )
		{
			Bar( s, s );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 6 )


def PassImmutableReferenceTwoTimes_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		fn Bar( S& a, S& b ) { }
		fn Foo( S& s )
		{
			Bar( s, s );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test0():
	c_program_text= """
		struct S{ i32 & x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
		fn DoPollution( S& mut s0, S &imut s1 ) @(pollution);
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			DoPollution( s, s_copy );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test1():
	c_program_text= """
		struct S
		{
			i32 & x;
			op=( mut this, S &imut other ) {} // Does reference pollution
		}
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			s= s_copy;
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test3():
	c_program_text= """
		struct S{ i32 & x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		fn DoPollution( S& mut s0, i32 & x ) @(pollution);
		fn Foo()
		{
			var i32 x= 0;
			var S mut s{ .x= x };
			auto s_copy = s;
			DoPollution( s, s_copy.x );
		}
	"""
	tests_lib.build_program( c_program_text )


def InnerReferencesChain_Test0():
	c_program_text= """
		struct S
		{
			i32 &mut x;
			i32 y;
			fn Bar(this) : bool;
			var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
			fn GetY(mut this) : i32 &mut @(return_references);
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];
		fn MakeS( i32 & mut x ) : S @(return_inner_references);
		fn Foo( S &mut s )
		{
			for( auto mut ss= MakeS( s.GetY() ); ss.Bar(); ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def InnerReferencesChain_Test1():
	c_program_text= """
		struct S
		{
			i32 &mut x;
			i32 y;
			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
			fn constructor( this, i32 & mut x ) @(pollution);
			fn Bar(this) : bool;
			var [ [ char8, 2 ], 1 ] return_references[ "0_" ];
			fn GetY(mut this) : i32  & mut @(return_references);
		}
		fn Foo( S &mut s )
		{
			for( var S mut ss( s.GetY() ); ss.Bar(); ){}
		}
	"""
	tests_lib.build_program( c_program_text )


def PollutionAndReturn_Test0():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		var [ [ char8, 2 ], 1 ] return_references[ "1_" ];
		fn DoPollution( S& mut s, i32 & mut x ) @(pollution) : i32 &mut @(return_references);
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s {.x= x };
			var i32 &mut y_ref= DoPollution( s, y ); // Creates two mutable references for 'y' - 'y_ref' and inside 's'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PollutionAndReturn_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1a" ] ];
		var [ [ char8, 2 ], 1 ] return_references[ "1a" ];
		fn DoPollution( S& mut s0, S& mut s1 ) @(pollution) : i32 &mut @(return_references);
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var S mut s0 {.x= x }, mut s1{ .x= y };
			var i32 &mut y_ref= DoPollution( s0, s1 ); // Creates two mutable references for 'y' - 'y_ref' and inside 's0'.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def DoublePollution_Test0():
	c_program_text= """
		struct S{ i32 &mut x; }
		var [ [ [char8, 2], 2 ], 2 ] pollution[ [ "0a", "2_" ], [ "1a", "2_" ] ];
		fn DoPollution( S& mut s0, S& mut s1, i32 & mut x ) @(pollution); // Call to this function will always produce error.
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			var S mut s0 {.x= x }, mut s1{ .x= y };
			DoPollution( s0, s1, z );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 9 )


def DoublePollution_Test1():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		var [ [ [char8, 2], 2 ], 2 ] pollution[ [ "0a", "2_" ], [ "1a", "2_" ] ];
		fn DoPollution( Smut& mut s0, Simut& mut s1, i32 & mut x ) @(pollution); // Call to this function will always produce error.
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0, mut z= 0;
			var Smut  mut s_mut { .x= x };
			var Simut mut s_imut{ .x= y };
			DoPollution( s_mut, s_imut, z );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 11 )


def TemporaryReferenceRemoving_Test0():
	c_program_text= """
		fn Pass( i32 &mut x ) : i32& mut;
		fn Bar( i32 &imut x, i32 &imut y );
		fn Foo()
		{
			var i32 mut x= 0;
			// Temporary mutable reference produced here in call of "Pass", but it is destroyed after binding it to "imut" param and this allow us later take "imut" reference for "x".
			Bar( Pass(x), x );
		}
	"""
	tests_lib.build_program( c_program_text )


def TemporaryReferenceRemoving_Test1():
	c_program_text= """
		fn Bar( i32 &imut x, i32 &imut y );
		fn Foo(bool c)
		{
			var i32 mut x= 0, mut y= 0;
			// Temporary mutable reference produced here as result of "select" operator, but it is destroyed after binding it to "imut" param and this allow us later take "imut" reference for "x".
			Bar( select(c ? x : y), select(c ? cast_imut(y) : cast_imut(x)) );
		}
	"""
	tests_lib.build_program( c_program_text )


def TemporaryReferenceRemoving_Test2():
	c_program_text= """
		fn Bar( i32 &imut x, i32 &imut y );
		fn Foo(bool c)
		{
			var [ i32, 2 ] mut arr= zero_init;
			// Temporary mutable reference produced here as result of "[]" operator, but it is destroyed after binding it to "imut" param and this allow us later take "imut" reference for "arr".
			Bar( arr[0], cast_imut(arr)[1] );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceFieldNode_Test2():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Bar( i32 &imut x, i32 &imut y );
		fn Foo(bool cond)
		{
			var i32 mut x= 0, mut y= 0;
			var S s0{ .r= x }, s1 { .r= y };
			auto& s_ref= select( cond ? s0 : s1 );
			// Temporary mutable reference is produced here for member access operator for reference. Because of that we get error when creating reference node while accessing ".r" second time.
			Bar( s_ref.r, s_ref.r );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def ReferenceInnerReferenceNode_Test0():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			auto& s_ref0= s; // Create here inner reference node for "s_ref0". Create link to inner reference node of "s".
			auto& s_ref1= s; // Create here inner reference node for "s_ref1". Create second mutable link to inner reference node of "s", which causes an error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceInnerReferenceNode_Test1():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			auto& s_ref0= s; // Create here inner reference node for "s_ref0". Create link to inner reference node of "s".
			var S& s_ref1= s; // Create here inner reference node for "s_ref1". Create second mutable link to inner reference node of "s", which causes an error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceInnerReferenceNode_Test2():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			auto& s_proxy_ref= s;
			var S& s_ref0= s_proxy_ref; // Create here inner reference node for "s_ref0". Create link to inner reference node of "s".
			var S& s_ref1= s_proxy_ref; // Create here inner reference node for "s_ref1". Create second mutable link to inner reference node of "s", which causes an error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceInnerReferenceNode_Test3():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			auto& s_ref0= s; // Create here inner reference node for "s_ref0". Create link to inner reference node of "s".
			cast_imut(s); // Create here inner reference node for result of "cast_imut". Create second mutable link to inner reference node of "s", which causes an error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceInnerReferenceNode_Test4():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Bar( S& s0, S& s1 );
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			Bar( s, s ); // Create here two inner reference nodes for two reference args, causing reference protection error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def ReferenceInnerReferenceNode_Test5():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Foo( bool cond )
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			var i32 mut y= 0;
			var S other_s{ .r= y };
			auto& s_ref= s;
			select( cond ? s : other_s ); // Create inner reference node for result of "select" and make to it link from "s". This is a second mutable link, which causes reference protection error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 10 ) )


def ReferenceInnerReferenceNode_Test6():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Bar( S& s );
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			auto& s_ref= s;
			Bar( s ); // Create here inner reference node for arg node, causing creation of second mutable link to inner node of "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceInnerReferenceNode_Test7():
	c_program_text= """
		struct S { i32 &mut r; }
		struct T { S s; }
		fn Foo()
		{
			var i32 mut x= 0;
			var T t{ .s{ .r= x } };
			auto& s_ref= t.s; // Create inner reference node, that is linked with "t" inner reference node.
			auto& t_ref= t; // Create inner reference node, that is linked with "t" inner reference node (second mutable link).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceInnerReferenceNode_Test8():
	c_program_text= """
		struct S { i32 &mut r; }
		struct T { S s; }
		fn Foo()
		{
			var i32 mut x= 0;
			var T t{ .s{ .r= x } };
			auto& t_ref= t; // Create inner reference node, that is linked with "t" inner reference node.
			auto& s_ref= t.s; // Create inner reference node, that is linked with "t" inner reference node (second mutable link).
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HaveError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 9 ) )


def ReferenceInnerReferenceNode_Test9():
	c_program_text= """
		struct S { i32 &mut r; }
		struct T { S s; }
		fn Bar( T& t, S& s );
		fn Bar( S& s, T& t );
		fn Foo()
		{
			var i32 mut x= 0;
			var T t{ .s{ .r= x } };
			Bar( t, t.s ); // Create two inner nodes, linked with inner node of "t".
			Bar( t.s, t ); // Create two inner nodes, linked with inner node of "t".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def OperatorsWithNodeLock_Test0():
	c_program_text= """
		fn Foo()
		{
			auto mut x= 0;
			with( &x_ref : x ) // Operator locks the value, preventing its moving.
			{
				move(x);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 7 ) )


def OperatorsWithNodeLock_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup[i32] mut t= zero_init;
			for( el : t ) // "for" for tuple prevents changing source tuple.
			{
				move(t);
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 7 ) )


def OperatorsWithNodeLock_Test2():
	c_program_text= """
		fn Foo( size_type i )
		{
			var [ i32, 4 ] mut arr= zero_init;
			arr[ move(arr)[i] ]; // indexation operator locks indexed variable and makes moving/chanhing of it impossible during index calculation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 5 ) )


def OperatorsWithNodeLock_Test3():
	c_program_text= """
		fn Foo( size_type i )
		{
			var [ i32, 4 ] mut arr= zero_init;
			arr == move(arr); // == operator for arrays locks variable and makes moving/chanhing of it impossible during index calculation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "MovedVariableHaveReferences", 5 ) )


def OperatorsWithNodeLock_Test4():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			move(arr) == arr; // variable used in the right part was previously moved in the left part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 5 ) )


def OperatorsWithNodeLock_Test5():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] mut arr= zero_init;
			arr= arr; // Right part is locked in the assignment operator calculation for an array is locked before calculating left part. This prevents self-assignment.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 5 ) )


def OperatorsWithNodeLock_Test6():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			t= move(t); // Accessing in the left part of assignment operaotr a variable moved in right part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "AccessingMovedVariable", 5 ) )
