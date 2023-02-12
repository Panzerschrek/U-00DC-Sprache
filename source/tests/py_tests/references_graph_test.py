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
		fn PassFirst( i32&'f x, i32&'s y ) : i32&'f { return x; }
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


def ReturnReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32 &imut r; }
		fn GetS( i32 &'a imut x ) : S'a'
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
	assert( errors_list[0].src_loc.line == 12 )


def ReturnReferenceFromStruct_Test0():
	c_program_text= """
		struct S{ i32 &mut r; }
		fn GetR( S& s'x' ) : i32 &'x mut
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
	assert( errors_list[0].src_loc.line == 12 )


def ReturnReferenceInsideStruct_Test0():
	c_program_text= """
		struct S{ i32& x; }
		fn GetS( i32&'r x ) : S'r'
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
	assert( errors_list[0].src_loc.line == 12 )


def PollutionTest0():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut s'dst', i32&'src x ) ' dst <- src' {}
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
	assert( errors_list[0].src_loc.line == 9 )


def PollutionTest1():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut s'dst', i32&'src x ) ' dst <- src' {}
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
	assert( errors_list[0].src_loc.line == 10 )


def PollutionTest2():
	c_program_text= """
		struct S{ i32& x; }
		fn Pollution( S &mut a'dst', S& b'src' ) ' dst <- src' {}
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
	assert( errors_list[0].src_loc.line == 12 )


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
			auto& s0= s;
			auto& s1= s;
			Bar( s0, s1 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


def PassMutableReferenceTwoTimes_Tes5():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn Bar( S a, S b ) { ++a.x; ++b.x; }
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .x= x };
			auto& s0= s;
			auto& s1= s;
			Bar( s0, s1 );
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "ReferenceProtectionError" )
	assert( errors_list[0].src_loc.line == 10 )


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
		fn DoPollution( S& mut s0'a', S &imut s1'b' ) ' a <- b ';
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
		fn DoPollution( S& mut s0'a', i32 &'b x ) ' a <- b ';
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
			fn GetY(mut this) : i32  &'this mut;
		}
		fn MakeS( i32 &'f mut x ) : S'f';
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
			fn constructor( this'b', i32 &'f mut x ) ' b <- f ';
			fn Bar(this) : bool;
			fn GetY(mut this) : i32  &'this mut;
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
		fn DoPollution( S& mut s'a', i32 &'b mut x ) ' a <- b ' : i32 &'b mut;
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
	assert( errors_list[0].src_loc.line == 8 )


def PollutionAndReturn_Test1():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn DoPollution( S& mut s0'a', S& mut s1'b' ) ' a <- b ' : i32 &'b mut;
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
	assert( errors_list[0].src_loc.line == 8 )


def DoublePollution_Test0():
	c_program_text= """
		struct S{ i32 &mut x; }
		fn DoPollution( S& mut s0'a', S& mut s1'a', i32 &'b mut x ) ' a <- b '; // Call to this function will always produce error.
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
	assert( errors_list[0].src_loc.line == 8 )


def DoublePollution_Test1():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		fn DoPollution( Smut& mut s0'a', Simut& mut s1'a', i32 &'b mut x ) ' a <- b '; // Call to this function will always produce error.
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
	assert( errors_list[0].src_loc.line == 10 )


def InnerReferenceMutabilityChanging_Test0():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		fn Foo()
		{
			var i32 mut o= 0, mut p= 0;
			var tup[ Smut, Simut ] mut ss[ { .x= o }, { .x= p } ];
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 7 )


def InnerReferenceMutabilityChanging_Test1():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t;
			PollutionMut ( t, x );
			PollutionImut( t, y ); // Change mut -> imut
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 12 )


def InnerReferenceMutabilityChanging_Test2():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t;
			PollutionImut( t, x );
			PollutionMut ( t, y ); // Change mut -> imut
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 12 )


def InnerReferenceMutabilityChanging_Test3():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t0, mut t1;
			PollutionImut( t0, x );
			PollutionMut ( t1, y );
			t0= t1; // Change imut -> mut in assignment operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 13 )


def InnerReferenceMutabilityChanging_Test4():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t0, mut t1;
			PollutionMut ( t0, x );
			PollutionImut( t1, y );
			t0= t1; // Change mut -> imut in assignment operator
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 13 )


def InnerReferenceMutabilityChanging_Test5():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t0, mut t1;
			PollutionImut( t0, x );
			PollutionMut ( t1, y );
			t0= move(t1); // Change imut -> mut in move-assignment
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 13 )


def InnerReferenceMutabilityChanging_Test6():
	c_program_text= """
		struct Smut { i32 & mut x; }
		struct Simut{ i32 &imut x; }
		type T= tup[ [ Smut, 0 ], [ Simut, 0 ] ];
		fn PollutionMut ( T &mut t'a', i32 &'b  mut x ) ' a <- b ';
		fn PollutionImut( T &mut t'a', i32 &'b imut x ) ' a <- b ';
		fn Foo()
		{
			var i32 mut x= 0, mut y= 0;
			var T mut t0, mut t1;
			PollutionMut ( t0, x );
			PollutionImut( t1, y );
			t0= move(t1); // Change mut -> imut in move-assignment
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( errors_list[0].error_code == "InnerReferenceMutabilityChanging" )
	assert( errors_list[0].src_loc.line == 13 )


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


def ReferenceFieldNode_Test0():
	c_program_text= """
		struct S { i32 &mut r; }
		fn Bar( i32 &imut x, i32 &imut y );
		fn Foo()
		{
			var i32 mut x= 0;
			var S s{ .r= x };
			// Temporary reference is not produced here for member access operator for reference, because only one inner reference node is accessible here.
			Bar( s.r, s.r );
		}
	"""
	tests_lib.build_program( c_program_text )


def ReferenceFieldNode_Test1():
	c_program_text= """
		fn Bar( i32 &imut x, i32 &imut y );
		struct S
		{
			i32 &mut r;

			fn Foo( this )
			{
				// Temporary reference is not produced here for member access operator for reference, because only one inner reference node is accessible here.
				Bar( r, r );
			}
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


def StructFieldChildNodes_Test0():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			// Ok - create mutable references to different fields of same struct.
			var i32 &mut x= s.x;
			var f32 &mut y= s.y;
			++x;
			y *= 2.0f;
			halt if( x != 1 );
			halt if( y != 2.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test1():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 5, .y= 7.0f };
			{
				// Ok - create mutable references to different fields of same struct, accessed by reference.
				auto &mut s_ref= s;
				var i32 &mut x= s_ref.x;
				var f32 &mut y= s_ref.y;
				++x;
				y *= 2.0f;
			}
			halt if( s.x != 6 );
			halt if( s.y != 14.0f );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test2():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 &mut x= s.x;
			var i32 &mut x_again= s.x; // Error - creating second mutable reference to same field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test3():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 &mut x= s.x;
			auto & s_whole= s; // Error - creating reference to whole struct with existing mutable reference to its field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test4():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			var i32 & x= s.x;
			var S &mut s_whole= s; // Error - creating mutable reference to whole struct with existing reference to its field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test5():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto &mut s_whole= s;
			auto & x= s.x; // Error - creating reference to struct field with existing mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test6():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & s_whole= s;
			auto &mut x= s.x; // Error - creating mutable reference to struct field with existing reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test7():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & s_whole= s;
			auto & x= s.x; // Ok - creating reference to struct field for struct with existing reference.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test8():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			auto & x= s.x;
			auto & s_whole= s; // Ok - creating reference to struct for struct with existing reference to field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test9():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.y ); // Ok - passing to function two mutable references to different fields of same struct.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test10():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( f32& mut x, f32 &mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.y, s.y ); // Error - passing to function two mutable references to same field of same struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test11():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& mut x, S& mut s ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s ); // Error - passing to function mutable reference to struct field and mutable reference to whole struct.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test12():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S& mut s, f32& mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function mutable reference to whole struct and mutable reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test13():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S& s, f32& mut y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function reference to whole struct and mutable reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test14():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S &mut s, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Error - passing to function mutable reference to whole struct and reference to struct field.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 11 ) )


def StructFieldChildNodes_Test15():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( S & s, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s, s.y ); // Ok - passing to function reference to whole struct and reference to struct field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test16():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, S& s ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s ); // Ok - passing to function reference to struct field and reference to whole struct.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test17():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, f32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.y ); // Ok - passing to function references to two struct fields.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test18():
	c_program_text= """
		struct S
		{
			i32 x;
			f32 y;
		}
		fn Bar( i32& x, i32& y ) {}
		fn Foo()
		{
			var S mut s{ .x= 0, .y= 1.0f };
			Bar( s.x, s.x ); // Ok - passing to function two references to same struct field.
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test19():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			{
				// Ok - create mutable references to different fields of same struct.
				var i32 &mut x= t.s.x;
				var f32 &mut y= t.s.y;
				var bool &mut z= t.z;
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t.s.x != 16 );
			halt if( t.s.y != 45.3f );
			halt if( t.z != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test20():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			{
				// Ok - create mutable references to different fields of same struct.
				var S &mut s= t.s;
				var i32 &mut x= s.x;
				var f32 &mut y= s.y;
				var bool &mut z= t.z;
				x= 16;
				y= 45.3f;
				z= false;
			}
			halt if( t.s.x != 16 );
			halt if( t.s.y != 45.3f );
			halt if( t.z != false );
		}
	"""
	tests_lib.build_program( c_program_text )
	tests_lib.run_function( "_Z3Foov" )


def StructFieldChildNodes_Test21():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 &mut x= t.s.x; // Error - creating mutable reference to "t.s" which already has one mutable reference.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test22():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S & s= t.s;
			var i32 &mut x= t.s.x; // Error - creating mutable reference to "t.s" which already has one reference.

		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test23():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 & x= t.s.x; // Error - creating reference to "t.s" which already has one mutable reference.
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test24():
	c_program_text= """
		struct S{ i32 x; f32 y; }
		struct T{ S s; bool z; }
		fn Foo()
		{
			var T mut t{ .s{ .x= 0, .y= 1.0f }, .z= false };
			var S &mut s= t.s;
			var i32 &mut x= s.x; // Ok - creating reference, linked to reference "s".
			var f32 &mut y= s.y; // Ok - creating reference, linked to reference "s".
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test25():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				// Ok - creating two mutable references to "this".
				var i32 &mut x_ref= x;
				var f32 &mut y_ref= y;
				x_ref= 0;
				y_ref= 0.0f;
			}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test26():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				var i32 &mut x_ref_again= x; // Error - creating second reference to same "this" field.
			}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test27():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				Bar(); // Error - creating mutable reference to "this" in call to thiscall method "Bar", when mutable reference to one of "this" fields exists.
			}
			fn Bar( mut this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test28():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var f32 &mut y_ref= y;
				Bar(); // Error - creating mutable reference to "this" in call to thiscall method "Bar", when reference to one of "this" fields exists.
			}
			fn Bar( mut this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test29():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var f32 &mut y_ref= y;
				Bar(); // Error - creating reference to "this" in call to thiscall method "Bar", when reference to one of "this" fields exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test30():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 & x_ref= x;
				Bar(); // Ok - calling method with immutable "this" when immutable reference to one of fields exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test31():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				this.y /= 1.0f; // Ok, modifying field of "this" using explicit "this", when mutable reference to other field exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test32():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				y /= 1.0f; // Ok, modifying field of "this" using implicit "this", when mutable reference to other field exists.
			}
			fn Bar( this ) {}
		}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test33():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				this.x /= 1; // Error - modifying field of "this" using explicit "this", when mutable reference to same field exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test34():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				var i32 &mut x_ref= x;
				x /= 1; // Error - modifying field of "this" using implicit "this", when mutable reference to same field exists.
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test35():
	c_program_text= """
		struct S
		{
			i32 x; f32 y;
			fn Foo( mut this )
			{
				auto &this_ref= this;
				x /= 1; // Error - modifying field of "this" using implicit "this", when mutable reference to whole "this".
			}
			fn Bar( this ) {}
		}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test36():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= Pass( s.x ); // Ok, pass mutable reference to field "x". Result reference will point only to field node.
		var f32 &mut y= s.y; // Ok create reference to anouther field.
	}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test37():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Pass( i32 &mut x ) : i32 &mut { return x; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= Pass( s.x ); // Ok, pass reference to field "x". Result reference will point only to field node.
		var i32 &mut x_again= s.x; // Error - create mutable reference to same field.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 8 ) )


def StructFieldChildNodes_Test38():
	c_program_text= """
	struct S { i32 x; f32 y; }
	fn Foo()
	{
		var S mut s{ .x= 0, .y= 1.0f };
		var i32& mut x= s.x;
		cast_imut(s); // Error, create intermediate immutable reference to whole "s" while mutable reference to "s.x" exists.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


def StructFieldChildNodes_Test39():
	c_program_text= """
	struct S { i32 x; i32 y; i32 z; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= select( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var i32& mut z_ref= s.z; // Ok, create reference to third field.
	}
	"""
	tests_lib.build_program( c_program_text )


def StructFieldChildNodes_Test40():
	c_program_text= """
	struct S { i32 x; i32 y; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= select( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var i32& mut x_ref= s.x; // Error, create reference to "s.x" again.
		var i32& mut y_ref= s.y; // Error, create reference to "s.y" again.
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) >= 2 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 6 ) )
	assert( HaveError( errors_list, "ReferenceProtectionError", 7 ) )


def StructFieldChildNodes_Test41():
	c_program_text= """
	struct S { i32 x; i32 y; }
	fn Foo( S& mut s, bool condition )
	{
		var i32 &mut ref= select( condition ? s.x : s.y ); // Create reference to boths fields "s.x" and "s.y" (acutally not, but logically create.
		var S& s_ref= s; // Error, create reference to whole "s".
	}
	"""
	errors_list= ConvertErrors( tests_lib.build_program_with_errors( c_program_text ) )
	assert( len(errors_list) > 0 )
	assert( HaveError( errors_list, "ReferenceProtectionError", 6 ) )
