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


def PassMutableReferenceTwoTimes_Test0():
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


def PassMutableReferenceTwoTimes_Test1():
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


def PassMutableReferenceTwoTimes_Test2():
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


def PassMutableReferenceTwoTimes_Test3():
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


def PassMutableReferenceTwoTimes_Test4():
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


def PassMutableReferenceTwoTimes_Test5():
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


def PassMutableReferenceTwoTimes_Test6():
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


def ReferencesLoop_Test4():
	c_program_text= """
		struct S
		{
			i32 & x;
			fn TakeCopy( this ) : S @(return_inner_references)
			{
				return this;
			}
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Foo( S &mut s ) : S @(return_inner_references)
		{
			var S res= s;
			s= s.TakeCopy(); // Link here inner reference node of "s" with inner reference node derived from itself.
			return res;
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferencesLoop_Test5():
	c_program_text= """
		struct S
		{
			i32& x;
		}
		struct T
		{
			S& s;
			fn MakeCopy( this ) : T @(return_inner_references);
			op=( mut this, T& other );
		}
		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0a" ] ];
		fn Foo( T &mut t )
		{
			var T t_copy= t.MakeCopy();
			t= t_copy;
			var S& s= t.s;
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
			// Temporary mutable reference produced here as result of ternary operator, but it is destroyed after binding it to "imut" param and this allow us later take "imut" reference for "x".
			Bar( (c ? x : y), (c ? cast_imut(y) : cast_imut(x)) );
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
			auto& s_ref= ( cond ? s0 : s1 );
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
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 7 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


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
			( cond ? s : other_s ); // Create inner reference node for result of ternary operator and make to it link from "s". This is a second mutable link, which causes reference protection error.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( not HasError( errors_list, "ReferenceProtectionError", 9 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


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
	assert( not HasError( errors_list, "ReferenceProtectionError", 8 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


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
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


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
	assert( HasError( errors_list, "MovedVariableHasReferences", 7 ) )


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
	assert( HasError( errors_list, "MovedVariableHasReferences", 7 ) )


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
	assert( HasError( errors_list, "MovedVariableHasReferences", 5 ) )


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
	assert( HasError( errors_list, "MovedVariableHasReferences", 5 ) )


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
	assert( HasError( errors_list, "AccessingMovedVariable", 5 ) )


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
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


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
	assert( HasError( errors_list, "AccessingMovedVariable", 5 ) )


def MoveAssignmentForDestinationWithReferences_Test0():
	c_program_text= """
		struct S{}
		fn Foo()
		{
			var S mut s;
			auto& s_ref= s;
			s= S(); // Move-assign value to "s" here. Can't do this, because "s" has an immutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def MoveAssignmentForDestinationWithReferences_Test1():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		fn GetS() : S;
		fn Foo()
		{
			var S mut s= zero_init;
			auto &mut s_ref= s;
			s= GetS(); // Move-assign value to "s" here. Can't do this, because "s" has a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )


def MoveAssignmentForDestinationWithReferences_Test2():
	c_program_text= """
		fn GetT() : tup[ f32 ];
		fn Foo()
		{
			var tup[ f32 ] mut t= zero_init;
			auto &imut t_ref= t;
			t= GetT(); // Move-assign value to "t" here. Can't do this, because "t" has an immutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def MoveAssignmentForDestinationWithReferences_Test3():
	c_program_text= """
		struct S
		{
			[ i32, 4 ]& r;
		}
		fn GetA() : [ i32, 4 ];
		fn Foo()
		{
			var [ i32, 4 ] mut a= zero_init;
			var S s{ .r= a };
			a= GetA(); // Move-assign value to "a" here. Can't do this, because "a" has an immutable reference inside "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 11 ) )


def MoveAssignmentForDestinationWithReferences_Test4():
	c_program_text= """
		struct S{}
		struct T{ S s; }
		fn GetT() : T;
		fn Foo()
		{
			var T mut t= zero_init;
			var S& s_ref= t.s;
			t= GetT(); // Move-assign value to "t" here. Can't do this, because "t" has an immutable reference to its field "s".
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def CompositeAssignmentForDestinationWithReferences_Test0():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] mut a= zero_init, b= zero_init;
			auto& a_ref= a;
			a= b; // Error, can't assign to "a", because there is an immutable reference to it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def CompositeAssignmentForDestinationWithReferences_Test1():
	c_program_text= """
		fn Foo()
		{
			var tup[ f32, i64, bool ] mut a= zero_init, b= zero_init;
			auto &mut a_ref= a[1];
			a= b; // Error, can't assign to "a", because there is a mutable reference to its element.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def CreatingMutableReferencesLoop_Test0():
	c_program_text= """
		struct S
		{
			i32 &mut @('a') x;
			i32 &mut @('a') y;

			fn constructor( i32 &mut a ) @(pollution)
				( x= a, y= x ) // Since "x" and "y" mutable references share same reference tag, it shouldn't be allowed to initialize one reference field using another reference field.
			{}

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CreatingMutableReferencesLoop", 8 ) )


def CreatingMutableReferencesLoop_Test1():
	c_program_text= """
		class Vec
		{
			fn clear( mut this );

			fn get_element( mut this ) : i32 &mut @(return_references);

			var [ [ char8, 2 ] , 1 ] return_references[ "0_" ];
		}

		struct S
		{
			Vec &mut @('a') vec_ref;
			i32 &mut @('a') el;

			fn constructor( Vec &mut v ) @(pollution)
				( vec_ref= v, el= vec_ref.get_element() ) // A mutable reference and another reference derived from it are stored in the same reference tag of a struct.
			{}

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}

		fn Foo()
		{
			var Vec mut v;
			var S s( v );
			s.vec_ref.clear(); // This call may invalidate "s.el" reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CreatingMutableReferencesLoop", 17 ) )


def CreatingMutableReferencesLoop_Test2():
	c_program_text= """
		class Vec
		{
		public:
			fn clear( mut this )
			{
				// It's possible to invalidate element references via this method.
			}

			fn get_element( mut this ) : i32 &mut @(return_references)
			{
				return x_; // This can be generally a reference to a heap-allocated element instead.
			}

		private:
			var [ [ char8, 2 ] , 1 ] return_references[ "0_" ];

		private:
			i32 x_= 0;
		}

		template</type T/>
		struct Ref
		{
			T &mut r;

			fn constructor();
			fn constructor( T &mut in_r ) @(pollution);

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}

		struct T
		{
			Ref</Vec/> @("a") vec_ref;
			Ref</i32/> @("a") int_ref;
		}

		fn MakeDerivedReverence( Vec &mut v ) : Ref</i32/> @(return_inner_references)
		{
			return Ref</i32/>( v.get_element() );
		}

		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];

		fn Foo()
		{
			var Vec mut v;
			var T mut t{ .vec_ref(v), .int_ref() };
			t.int_ref= MakeDerivedReverence( t.vec_ref.r ); // This shouldn't be allowed.
			t.vec_ref.r.clear(); // Perform possible invalidation.
			auto& el= t.int_ref.r; // Accessing possibly-invalidated reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CreatingMutableReferencesLoop", 50 ) )


def CreatingMutableReferencesLoop_Test3():
	c_program_text= """
		class Vec
		{
		public:
			fn clear( mut this )
			{
				// It's possible to invalidate element references via this method.
			}

			fn get_element( mut this ) : i32 &mut @(return_references)
			{
				return x_; // This can be generally a reference to a heap-allocated element instead.
			}

		private:
			var [ [ char8, 2 ] , 1 ] return_references[ "0_" ];

		private:
			i32 x_= 0;
		}

		template</type T/>
		struct Ref
		{
			T &mut r;

			fn constructor();
			fn constructor( T &mut in_r ) @(pollution);

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}

		struct T
		{
			Ref</Vec/> @("a") vec_ref;
			Ref</i32/> @("a") int_ref;
		}

		fn MakeDerivedReverence( Vec &mut v ) : Ref</i32/> @(return_inner_references)
		{
			return Ref</i32/>( v.get_element() );
		}

		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];

		fn Bar( T &mut t )
		{
			t.int_ref= MakeDerivedReverence( t.vec_ref.r ); // This shouldn't be allowed.
			t.vec_ref.r.clear(); // Perform possible invalidation.
			auto& el= t.int_ref.r; // Accessing possibly-invalidated reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CreatingMutableReferencesLoop", 48 ) )


def CreatingMutableReferencesLoop_Test4():
	c_program_text= """
		class Vec
		{
		public:
			fn clear( mut this )
			{
				// It's possible to invalidate element references via this method.
			}

			fn get_element( mut this ) : i32 &mut @(return_references)
			{
				return x_; // This can be generally a reference to a heap-allocated element instead.
			}

		private:
			var [ [ char8, 2 ] , 1 ] return_references[ "0_" ];

		private:
			i32 x_= 0;
		}

		template</type T/>
		struct Ref
		{
			T &mut r;

			fn constructor();
			fn constructor( T &mut in_r ) @(pollution);
			op=( mut this, Ref</T/>& other );

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}

		struct T
		{
			Ref</Vec/> @("a") vec_ref;
			[ Ref</i32/>, 1 ] @("a") int_ref;
		}

		fn MakeDerivedReverence( Vec &mut v ) : [ Ref</i32/>, 1 ] @(return_inner_references)
		{
			var [ Ref</i32/>, 1 ] res[ ( v.get_element() ) ];
			return res;
		}

		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];

		fn Bar( T &mut t )
		{
			var [ Ref</i32/>, 1 ] mut i_ref= MakeDerivedReverence( t.vec_ref.r );
			t.int_ref= i_ref; // This shouldn't be allowed.
			move(i_ref);
			t.vec_ref.r.clear(); // Perform possible invalidation.
			auto& el= t.int_ref[0].r; // Accessing possibly-invalidated reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "CreatingMutableReferencesLoop", 51 ) )


def CreatingMutableReferencesLoop_Test5():
	c_program_text= """
		struct S
		{
			i32 &imut @('a') x;
			i32 &imut @('a') y;

			fn constructor( i32 &mut a ) @(pollution)
				( x= a, y= x ) // Fine, initializing an immutable reference field with other immutable reference field using same reference tag.
			{}

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}
	"""
	tests_lib.build_program( c_program_text )


def CreatingMutableReferencesLoop_Test6():
	c_program_text= """
		class Vec
		{
		public:
			fn get_element( this ) : i32 &imut @(return_references)
			{
				return x_; // This can be generally a reference to a heap-allocated element instead.
			}

		private:
			var [ [ char8, 2 ] , 1 ] return_references[ "0_" ];

		private:
			i32 x_= 0;
		}

		template</type T/>
		struct Ref
		{
			T &imut r;

			fn constructor();
			fn constructor( T &imut in_r ) @(pollution);

			var [ [ [char8, 2], 2 ], 1 ] pollution[ [ "0a", "1_" ] ];
		}

		struct T
		{
			Ref</Vec/> @("a") vec_ref;
			Ref</i32/> @("a") int_ref;
		}

		fn MakeDerivedReverence( Vec &imut v ) : Ref</i32/> @(return_inner_references)
		{
			return Ref</i32/>( v.get_element() );
		}

		var tup[ [ [ char8, 2 ], 1 ] ] return_inner_references[ [ "0_" ] ];

		fn Bar( T &mut t )
		{
			t.int_ref= MakeDerivedReverence( t.vec_ref.r ); // Ok, creating loop using immutable inner reference tags is allowed.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariableHavingMutableReference_Test0():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			var i32 y= x; // Reading variable having a mutable reference in variable expression initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test1():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			var i32 y( x ); // Reading variable having a mutable reference in variable constructor initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test2():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			var i32 y= x; // Reading variable having a mutable reference in auto variable initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test3():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			var S s_copy= s; // Reading variable having a mutable reference in variable expression initializer.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test4():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			var S s_copy( s ); // Reading variable having a mutable reference in variable constructor initializer.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test5():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto s_copy= s; // Reading variable having a mutable reference in auto variable initializer.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test6():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			var S s_copy= s; // Reading variable having a mutable reference in variable expression initializer.
		}
		type S= tup[ i32, [ f32, 4 ], bool ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test7():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			var S s_copy( s ); // Reading variable having a mutable reference in variable constructor initializer.
		}
		type S= tup[ i32, [ f32, 4 ], bool ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test8():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto s_copy= s; // Reading variable having a mutable reference in auto variable initializer.
		}
		type S= tup[ i32, [ f32, 4 ], bool ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test9():
	c_program_text= """
		fn Foo()
		{
			var E mut x= E::A;
			var E &mut x_ref= x;
			var E y= x; // Reading variable having a mutable reference in variable expression initializer.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test10():
	c_program_text= """
		fn Foo()
		{
			var E mut x= E::A;
			var E &mut x_ref= x;
			var E y( x ); // Reading variable having a mutable reference in variable constructor initializer.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test11():
	c_program_text= """
		fn Foo()
		{
			var E mut x= E::A;
			var E &mut x_ref= x;
			var E y= x; // Reading variable having a mutable reference in auto variable initializer.
		}
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test12():
	c_program_text= """
		fn Foo()
		{
			var bool mut b= false;
			auto &mut b_ref= b;
			if(b) // Reading variable having a mutable reference in if operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test13():
	c_program_text= """
		fn Foo( bool mut b )
		{
			auto &mut b_ref= b;
			while(b) // Reading variable having a mutable reference in while operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test14():
	c_program_text= """
		fn Foo( bool mut b )
		{
			auto &mut b_ref= b;
			for( ;b; ) // Reading variable having a mutable reference in C-style for operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test15():
	c_program_text= """
		fn Foo( bool mut b )
		{
			auto &mut b_ref= b;
			halt if(b); // Reading variable having a mutable reference in "halt if" operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test16():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			switch(x) // Reading variable having a mutable reference in switch operator.
			{
				0 -> {},
				22 -> {},
				default -> {},
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test17():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 0u;
			var u32 &mut x_ref= x;
			var u32 mut y= 0u;
			y*= x; // Reading variable having a mutable reference in compaund assignment operator.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test18():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 0u;
			var u32 &mut x_ref= x;
			x-= 33u; // Modifying variable having a mutable reference in compaund assignment operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test19():
	c_program_text= """
		fn Foo()
		{
			var size_type mut x= 0s;
			var size_type &mut x_ref= x;
			var $(f64) mut ptr= zero_init;
			unsafe{ ptr+= x; } // Reading variable having a mutable reference in compound assignment operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test20():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 0u;
			var u32 &mut x_ref= x;
			var u32 mut y= 0u;
			y= x; // Reading variable having a mutable reference in assignment operator.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test21():
	c_program_text= """
		fn Foo() : i32
		{
			var i32 mut x= 0;
			auto &mut x_ref= x;
			return x; // Reading variable having a mutable reference in return operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test22():
	c_program_text= """
		fn Foo() : S
		{
			var S mut s= zero_init;
			auto &mut s_ref= s;
			return s; // Reading variable having a mutable reference in return operator.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test23():
	c_program_text= """
		fn Foo() : T
		{
			var T mut t= zero_init;
			auto &mut t_ref= t;
			return t; // Reading variable having a mutable reference in return operator.
		}
		type T= [ tup[ f32, bool ], 3 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test24():
	c_program_text= """
		fn Foo()
		{
			var size_type mut s= 16s;
			var size_type &mut s_ref= s;
			alloca f32 arr[ s ]; // Reading variable having a mutable reference in alloca declaration.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test25():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto y= -x; // Reading variable having a mutable reference in unary minus.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test26():
	c_program_text= """
		fn Foo( i32 mut x )
		{
			var i32 &mut x_ref= x;
			auto y= ~x; // Reading variable having a mutable reference in unary bitwise not.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test27():
	c_program_text= """
		fn Foo()
		{
			var bool mut b= true;
			auto &mut b_ref= b;
			var bool not_b= !b; // Reading variable having a mutable reference in logical not.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test28():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 1u;
			auto &mut x_ref= x;
			var [ i32, 4 ] arr= zero_init;
			auto arr_x= arr[ x ]; // Reading variable having a mutable reference in indexation operator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test29():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 1u;
			auto &mut x_ref= x;
			auto sum= x + 3u; // Reading variable having a mutable reference in binary operator left part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test30():
	c_program_text= """
		fn Foo()
		{
			var u32 mut x= 1u;
			auto &mut x_ref= x;
			auto sum= 3u + x; // Reading variable having a mutable reference in binary operator right part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test31():
	c_program_text= """
		fn Foo( bool mut cond )
		{
			auto &mut cond_ref= cond;
			auto x= ( cond ? 55 : 44 ); // Reading variable having a mutable reference in ternary operator condition.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test32():
	c_program_text= """
		fn Foo( bool cond )
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto y= ( cond ? x : 44 ); // Reading variable having a mutable reference in ternary operator true part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test33():
	c_program_text= """
		fn Foo( bool cond )
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto y= ( cond ? 44 : x ); // Reading variable having a mutable reference in ternary operator false part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test34():
	c_program_text= """
		fn Foo()
		{
			var bool mut b0= false, imut b1= false;
			var bool &mut b0_ref= b0;
			var bool cond= b0 && b1; // Reading variable having a mutable reference in lazy logical and left part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test35():
	c_program_text= """
		fn Foo()
		{
			var bool mut b0= false, imut b1= false;
			var bool &mut b0_ref= b0;
			var bool cond= b0 || b1; // Reading variable having a mutable reference in lazy logical or left part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test36():
	c_program_text= """
		fn Foo()
		{
			var bool imut b0= false, mut b1= false;
			var bool &mut b1_ref= b1;
			var bool cond= b0 && b1; // Reading variable having a mutable reference in lazy logical and right part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test37():
	c_program_text= """
		fn Foo()
		{
			var bool imut b0= false, mut b1= false;
			var bool &mut b1_ref= b1;
			var bool cond= b0 || b1; // Reading variable having a mutable reference in lazy logical or right part.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test38():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var $(i32) mut x_ptr= $<(x);
			var $(i32) &mut x_ptr_ref= x_ptr;
			unsafe
			{
				auto& x_ref= $>(x_ptr); // Reading variable having a mutable reference in raw pointer dereference.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def AccessingVariableHavingMutableReference_Test39():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] mut a0= zero_init, imut a1= zero_init;
			auto &mut a0_ref= a0;
			var bool eq= a0 == a1; // Reading variable having a mutable reference in left part of == operator for arrays.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test40():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] imut a0= zero_init, mut a1= zero_init;
			auto &mut a1_ref= a1;
			var bool eq= a0 == a1; // Reading variable having a mutable reference in left part of == operator for arrays.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test41():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] mut a0= zero_init, imut a1= zero_init;
			auto &mut a0_ref= a0;
			var bool eq= a0 != a1; // Reading variable having a mutable reference in left part of != operator for arrays.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test42():
	c_program_text= """
		fn Foo()
		{
			var [ i32, 4 ] imut a0= zero_init, mut a1= zero_init;
			auto &mut a1_ref= a1;
			var bool eq= a0 != a1; // Reading variable having a mutable reference in left part of != operator for arrays.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test43():
	c_program_text= """
		fn Foo()
		{
			var [ char8, 4 ] mut a0= zero_init, imut a1= zero_init;
			auto &mut a0_ref= a0;
			auto res= a0 + a1; // Reading variable having a mutable reference in left part of char arrays concatenation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test44():
	c_program_text= """
		fn Foo()
		{
			var [ char8, 4 ] imut a0= zero_init, mut a1= zero_init;
			auto &mut a1_ref= a1;
			auto res= a0 + a1; // Reading variable having a mutable reference in right part of char arrays concatenation.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test45():
	c_program_text= """
		fn Foo()
		{
			var (fn()) mut fn_ptr= Bar;
			auto &mut fn_ptr_ref= fn_ptr;
			fn_ptr(); // Reading variable having a mutable reference in function pointer call.
		}
		fn Bar();
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test46():
	c_program_text= """
		fn Foo()
		{
			var S mut s= 0;
			var S &mut s_ref= s;
			Bar( s ); // Reading variable having a mutable reference while reading argument for function call.
		}
		fn Bar( S s );
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test47():
	c_program_text= """
		fn Foo()
		{
			var S mut s= 0;
			var S &mut s_ref= s;
			Bar( s ); // Reading variable having a mutable reference while taking reference for argument for function call.
		}
		fn Bar( S& s );
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test48():
	c_program_text= """
		fn Foo()
		{
			var T mut t= 0;
			var T &mut t_ref= t;
			Bar( t ); // Reading variable having a mutable reference while reading argument for function call.
		}
		fn Bar( T t );
		type T= tup[ bool, char8, f64, u32 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test49():
	c_program_text= """
		fn Foo()
		{
			var T mut t= 0;
			var T &mut t_ref= t;
			Bar( t ); // Reading variable having a mutable reference while taking reference for argument for function call.
		}
		fn Bar( T& t );
		type T= tup[ bool, char8, f64, u32 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test50():
	c_program_text= """
		fn generator Foo() : i32
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			yield x; // Reading variable having a mutable reference in "yield" operator for a generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test51():
	c_program_text= """
		fn generator Foo() : i32
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			return x; // Reading variable having a mutable reference in "return" operator for a generator.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test52():
	c_program_text= """
		fn generator Foo() : S
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			yield s; // Reading variable having a mutable reference in "yield" operator for a generator.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test53():
	c_program_text= """
		fn generator Foo() : S
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			return s; // Reading variable having a mutable reference in "return" operator for a generator.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test54():
	c_program_text= """
		fn generator Foo() : A
		{
			var A mut a= zero_init;
			var A &mut a_ref= a;
			yield a; // Reading variable having a mutable reference in "yield" operator for a generator.
		}
		type A= [ tup[ bool, u64 ], 13 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test55():
	c_program_text= """
		fn generator Foo() : A
		{
			var A mut a= zero_init;
			var A &mut a_ref= a;
			return a; // Reading variable having a mutable reference in "return" operator for a generator.
		}
		type A= [ tup[ bool, u64 ], 13 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test56():
	c_program_text= """
		fn async Foo() : i32
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			return x; // Reading variable having a mutable reference in "return" operator for an async function.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test57():
	c_program_text= """
		fn async Foo() : S
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			return s; // Reading variable having a mutable reference in "return" operator for an async function.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test58():
	c_program_text= """
		fn async Foo() : A
		{
			var A mut a= zero_init;
			var A &mut a_ref= a;
			return a; // Reading variable having a mutable reference in "return" operator for an async function.
		}
		type A= [ tup[ bool, u64 ], 13 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test59():
	c_program_text= """
		fn Foo()
		{
			var ( generator : i32 ) mut g= Gen();
			auto &mut g_ref= g;
			if_coro_advance( res : g ) // Reading generator variable having a mutable reference in "if_coro_advance" operator.
			{}
		}
		fn generator Gen() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test60():
	c_program_text= """
		fn Foo()
		{
			var ( async : i32 ) mut f= AsyncFunc();
			auto &mut f_ref= f;
			if_coro_advance( res : f ) // Reading async function variable having a mutable reference in "if_coro_advance" operator.
			{}
		}
		fn async AsyncFunc() : i32;
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test61():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[&]() : i32 { return x; }; // Implicitly capture by reference a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test62():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[=]() : i32 { return x; }; // Implicitly capture by value a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test63():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[&x]() : i32 { return x; }; // Explicitly capture by reference a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test64():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[x]() : i32 { return x; }; // Explicitly capture by value a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test65():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[&x_ref_l= x]() : i32 { return x_ref_l; }; // Create reference named capture for a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test66():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto l= lambda[x_copy= x]() : i32 { return x_copy; }; //Create value named capture a variable having a mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test67():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[&]() : S { return s; }; // Implicitly capture by reference a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test68():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[=]() : S { return s; }; // Implicitly capture by value a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test69():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[&s]() : S { return s; }; // Explicitly capture by reference a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test70():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[s]() : S { return s; }; // Explicitly capture by value a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test71():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[&s_ref_l= s]() : S { return s_ref_l; }; // Create reference named capture for a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test72():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto l= lambda[s_copy= s]() : S { return s_copy; }; //Create value named capture a variable having a mutable reference.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test73():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[&]() : T { return t; }; // Implicitly capture by reference a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test74():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[=]() : T { return t; }; // Implicitly capture by value a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test75():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[&t]() : T { return t; }; // Explicitly capture by reference a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test76():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[t]() : T { return t; }; // Explicitly capture by value a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test77():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[&t_ref_l= t]() : T { return t_ref_l; }; // Create reference named capture for a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test78():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			var T &mut t_ref= t;
			auto l= lambda[t_copy= t]() : T { return t_copy; }; //Create value named capture a variable having a mutable reference.
		}
		type T= tup[ [ i32, 2 ], f32, bool, u64, char8 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test79():
	c_program_text= """
		fn Foo()
		{
			var (fn()) mut fn_ptr= Bar;
			auto &mut fn_ptr_ref= fn_ptr;
			var S s{ .fn_ptr= fn_ptr }; // Reading variable having a mutable reference in function pointer field initialization via expression initializer.
		}
		fn Bar();
		struct S{ (fn()) fn_ptr; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test80():
	c_program_text= """
		fn Foo()
		{
			var (fn()) mut fn_ptr= Bar;
			auto &mut fn_ptr_ref= fn_ptr;
			var S s{ .fn_ptr( fn_ptr ) }; // Reading variable having a mutable reference in function pointer field initialization via constructor initializer.
		}
		fn Bar();
		struct S{ (fn()) fn_ptr; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test81():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut x_ref= x;
			var S s{ .x= x }; // Reading variable having a mutable reference in field initialization via expression initializer.
		}
		struct S{ i32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test82():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			auto &mut x_ref= x;
			var S s{ .x( x ) }; // Reading variable having a mutable reference in field initialization via constructor initializer with type conversion.
		}
		struct S{ f32 x; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test83():
	c_program_text= """
		fn Foo()
		{
			var E mut e= E::B;
			auto &mut e_ref= e;
			var S s{ .e= e }; // Reading variable having a mutable reference in field initialization via expression initializer.
		}
		struct S{ E e; }
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test84():
	c_program_text= """
		fn Foo()
		{
			var E mut e= E::B;
			auto &mut e_ref= e;
			var S s{ .e= e }; // Reading variable having a mutable reference in field initialization via constructor initializer.
		}
		struct S{ E e; }
		enum E{ A, B, C }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test85():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			auto &mut t_ref= t;
			var S s{ .t= t }; // Reading variable having a mutable reference in field initialization via expression initializer.
		}
		struct S{ T t; }
		type T= [ tup[ f32, bool, [ i32, 3 ], S ], 4 ];
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test86():
	c_program_text= """
		fn Foo()
		{
			var T mut t= zero_init;
			auto &mut t_ref= t;
			var S s{ .t( t ) }; // Reading variable having a mutable reference in field initialization via constructor initializer.
		}
		struct S{ T t; }
		type T= [ tup[ f32, bool, [ i32, 3 ], S ], 4 ];
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test87():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto y= f32(x); //  Reading variable having a mutable reference in type conversion.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test88():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			auto s= S(x); // Reading variable having a mutable reference in temp variable construction.
		}
		struct S
		{
			i32 x;
			fn constructor( i32 in_x )
				( x= in_x )
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test89():
	c_program_text= """
		class A
		{
			i32 &mut x;
			i32 y;
			fn constructor( i32 &mut in_x )
				( x= in_x, y= in_x ) // Reading variable having a mutable reference in field initializer.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test90():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			with( x_copy : x )  // Reading variable having a mutable reference in "with" operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test91():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			with( &another_x_ref : x ) // Creating a reference to a variable having a mutable reference in "with" operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test92():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			with( &mut another_x_ref : x ) // Creating a mutable reference to a variable having a mutable reference in "with" operator.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test93():
	c_program_text= """
		fn Foo( S mut s )
		{
			var S &mut s_ref= s;
			with( s_copy : s ) // Reading variable having a mutable reference in "with" operator.
			{}
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test94():
	c_program_text= """
		fn Foo( S mut s )
		{
			var S &mut s_ref= s;
			with( &another_s_ref : s ) // Creating a reference to a variable having a mutable reference in "with" operator.
			{}
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test95():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			with( &mut another_s_ref : s ) // Creating a mutable reference to a variable having a mutable reference in "with" operator.
			{}
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test96():
	c_program_text= """
		fn Foo( A mut a )
		{
			var A &mut a_ref= a;
			with( a_copy : a ) // Reading variable having a mutable reference in "with" operator.
			{}
		}
		type A= [ tup[ f32, f64, bool, char16 ], 16 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test97():
	c_program_text= """
		fn Foo( A mut a )
		{
			var A &mut a_ref= a;
			with( &another_a_ref : a ) // Creating a reference to a variable having a mutable reference in "with" operator.
			{}
		}
		type A= [ tup[ f32, f64, bool, char16 ], 16 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test98():
	c_program_text= """
		fn Foo()
		{
			var A mut a= zero_init;
			var A &mut a_ref= a;
			with( &mut another_a_ref : a ) // Creating a mutable reference to a variable having a mutable reference in "with" operator.
			{}
		}
		type A= [ tup[ f32, f64, bool, char16 ], 16 ];
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test99():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var S &mut s_ref= s;
			auto x= s.x; // Reading a struct field while having a mutable reference to the whole struct.
			auto y= s.y; // Reading a struct field while having a mutable reference to the whole struct.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test100():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var i32 &mut x_ref= s.x;
			auto s_copy= s; // Reading the whole struct while having a mutable reference to one of its fields.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test101():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var f32 &mut y_ref= s.y;
			auto y= s.y; // Reading struct field while having a mutable reference to it.
		}
		struct S{ i32 x; f32 y; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test102():
	c_program_text= """
		fn Foo()
		{
			var S mut s= zero_init;
			var f32 &mut y_ref= s.y;
			auto x= s.x; // Reading struct field while having a mutable reference to anoter field - this should be fine.
		}
		struct S{ i32 x; f32 y; }
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariableHavingMutableReference_Test103():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			var tup[ i32, f32 ] &mut t_ref= t;
			auto x= t[0]; // Reading a tuple member while having a mutable reference to the whole tuple.
			auto y= t[1]; // Reading a tuple member while having a mutable reference to the whole tuple.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test104():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			var i32 &mut x_ref= t[0];
			auto t_copy= t; // Reading the whole tuple while having a mutable reference to one of its members.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test105():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			var f32 &mut y_ref= t[1];
			auto y= t[1]; // Reading tuple member while having a mutable reference to it.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test106():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			var f32 &mut y_ref= t[1];
			auto x= t[0]; // Reading tuple member while having a mutable reference to anoter member - this should be fine.
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariableHavingMutableReference_Test108():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			auto &mut t_ref= t;
			for( el : t ) // Reading members of tuple in "for" operator for a variable having a mutable reference.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test109():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			auto &mut t_ref= t;
			for( &el : t ) // Creating a reference for members of tuple in "for" operator for a variable having a mutable reference.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test110():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, f32 ] mut t= zero_init;
			auto &mut t_ref= t;
			for( &mut el : t ) // Creating a reference for members of tuple in "for" operator for a variable having a mutable reference.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test111():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, tup[ f32, bool ] ] mut t= zero_init;
			var bool &mut b_ref= t[1][1];
			for( el : t ) // Reading members of tuple in "for" operator for a variable having a mutable reference to one of its members.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test112():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, tup[ f32, bool ] ] mut t= zero_init;
			var bool &mut b_ref= t[1][1];
			for( &el : t ) // Creating a reference for members of tuple in "for" operator for a mutable reference to one of its members.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test113():
	c_program_text= """
		fn Foo()
		{
			var tup[ i32, tup[ f32, bool ] ] mut t= zero_init;
			var bool &mut b_ref= t[1][1];
			for( &mut el : t ) // Creating a reference for members of tuple in "for" operator for a mutable reference to one of its members.
			{}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 6 ) )


def AccessingVariableHavingMutableReference_Test114():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref= x;
			type T= typeof(x); // Fine, "typeof" doesn't perform memory access, so it's safe even if "x" has a mutable reference.
			static_assert( same_type</T, i32/> );
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariableHavingMutableReference_Test115():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
			fn Foo( mut this )
			{
				auto &mut this_ref= this;
				auto x_copy= x; // Reading a struct field while having a mutable reference to the whole struct.
				auto y_copy= y; // Reading a struct field while having a mutable reference to the whole struct.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError",  9 ) )
	assert( HasError( errors_list, "ReferenceProtectionError", 10 ) )


def AccessingVariableHavingMutableReference_Test116():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				auto this_copy= this; // Reading the whole struct while having a mutable reference to one of its fields.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def AccessingVariableHavingMutableReference_Test117():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
			fn Foo( mut this )
			{
				var f32 &mut y_ref= y;
				auto y_copy= y; // Reading struct field while having a mutable reference to it.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 9 ) )


def AccessingVariableHavingMutableReference_Test118():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
			fn Foo( mut this )
			{
				var f32 &mut y_ref= y;
				auto x_copy= x; // Reading struct field while having a mutable reference to anoter field - this should be fine.
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def AccessingVariableHavingMutableReference_Test119():
	c_program_text= """
		fn Foo()
		{
			var i32 mut x= 0;
			var i32 &mut x_ref0= x;
			var i32 &mut x_ref1= x_ref0;
			var i32 y= x_ref0; // Reading reference having a mutable reference in variable expression initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 7 ) )


def AccessingVariableHavingMutableReference_Test120():
	c_program_text= """
		fn Foo( i32 &mut x )
		{
			var i32 &mut x_ref= x;
			var i32 y= x; // Reading reference having a mutable reference in variable expression initializer.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test121():
	c_program_text= """
		fn Foo( i32 &mut x )
		{
			var R r{ .r= x };
			var i32 y= x; // Reading reference having a mutable reference in variable expression initializer.
		}
		struct R{ i32 &mut r; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test122():
	c_program_text= """
		fn Foo( R r )
		{
			auto &mut r_ref= r.r;
			var i32 y= r.r; // Reading reference having a mutable reference in variable expression initializer.
		}
		struct R{ i32 &mut r; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test123():
	c_program_text= """
		fn Foo( R& r )
		{
			auto &mut r_ref= r.r;
			var i32 y= r.r; // Reading reference having a mutable reference in variable expression initializer.
		}
		struct R{ i32 &mut r; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test124():
	c_program_text= """
		fn Foo( R mut r )
		{
			var R &mut r_ref= r;
			var i32& int_ref= r.r; // Reading reference field of a variable having a mutable reference.
		}
		struct R{ i32 &imut r; }
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 5 ) )


def AccessingVariableHavingMutableReference_Test125():
	c_program_text= """
		struct R
		{
			i32 &imut r;
			fn Foo( mut this )
			{
				var R &mut this_ref= this;
				var i32& int_ref= r; // Reading reference field of a variable having a mutable reference.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HasError( errors_list, "ReferenceProtectionError", 8 ) )
